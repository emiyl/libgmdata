#include "common.h"
#include <math.h>

static int PATH_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int PATH_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
static int PathPoint_parse(Reader *reader, PathPoint *point);
static void GamePath_computeInternal(GamePath* path);

int PATH_parse(DataWin *dw) {
    Chunk chunk = {0};
    PathChunk *p = &dw->path;

    if (get_chunk(dw, "PATH", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "PATH");

    return Reader_readAndParsePointerTable(
        &reader, dw,
        (void **)&p->paths, NULL,
        &p->count, sizeof(GamePath),
        PATH_pointerTable_parse,
        PATH_pointerTable_missingHandler,
        NULL
    );
}

static int GamePath_parse(Reader *reader, DataWin *dw, GamePath *path) {
    path->present = true;
    path->internalPoints = NULL;
    path->internalPointCount = 0;
    path->length = 0.0;
    Reader_readString(reader, dw, &path->name);
    Reader_readBool32(reader, &path->isSmooth);
    Reader_readBool32(reader, &path->isClosed);
    Reader_readUInt32(reader, &path->precision);
    path->exists = true;

    // Points SimpleList
    Reader_readUInt32(reader, &path->pointCount);
    if (path->pointCount > 0) {
        path->points = (PathPoint*)safeMalloc(sizeof(PathPoint) * path->pointCount);
        repeat(path->pointCount, i) {
            if (PathPoint_parse(reader, &path->points[i]) != 0) {
                free(path->points);
                path->points = NULL;
                path->pointCount = 0;
                return -1;
            }
        }
    } else {
        path->points = NULL;
    }

    GamePath_computeInternal(path);

    return 0;
}

static int PathPoint_parse(Reader *reader, PathPoint *point) {
    Reader_readFloat32(reader, &point->x);
    Reader_readFloat32(reader, &point->y);
    Reader_readFloat32(reader, &point->speed);
    return 0;
}

static int PATH_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)extraData; // Unused parameter
    return GamePath_parse(reader, dw, (GamePath *)out);
}

static int PATH_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[PATH_pointerTable_missingHandler] Path pointer is missing, initializing default values.\n");

    GamePath *path = (GamePath *)out;
    path->present = false;
    path->name = NULL;
    path->isSmooth = false;
    path->isClosed = false;
    path->precision = 0;
    path->exists = false;
    path->pointCount = 0;
    path->points = NULL;
    path->internalPoints = NULL;
    path->internalPointCount = 0;
    path->length = 0.0;

    return 0;
}

static int GamePath_free(GamePath *path) {
    free(path->points);
    path->points = NULL;
    path->pointCount = 0;

    free(path->internalPoints);
    path->internalPoints = NULL;
    path->internalPointCount = 0;
    path->length = 0.0;

    return 0;
}

int PATH_free(PathChunk *path) {
    int result = 0;
    repeat(path->count, i) {
        if (GamePath_free(&path->paths[i])) {
            logWarn("[PATH_free] Failed to free GamePath at index %u\n", i);
            result = -1;
        }
    }
    free(path->paths);
    path->paths = NULL;
    path->count = 0;
    return result;
}

static InternalPathPoint *tempIntPoints = NULL;
static uint32_t tempIntPointCount = 0;
static uint32_t tempIntPointCapacity = 0;

static int tempIntPoints_free(void) {
    free(tempIntPoints);
    tempIntPoints = NULL;
    tempIntPointCount = 0;
    tempIntPointCapacity = 0;
    return 0;
}

static void addInternalPoint(float x, float y, float speed) {
    if (tempIntPointCount >= tempIntPointCapacity) {
        uint32_t newCapacity = tempIntPointCapacity
            ? tempIntPointCapacity * 2
            : 16;

        InternalPathPoint *newPoints =
            realloc(tempIntPoints, newCapacity * sizeof(*newPoints));

        if (newPoints == NULL) {
            free(tempIntPoints);
            tempIntPoints = NULL;
            tempIntPointCount = 0;
            tempIntPointCapacity = 0;
            abort();
        }

        tempIntPoints = newPoints;
        tempIntPointCapacity = newCapacity;
    }

    InternalPathPoint *pt = &tempIntPoints[tempIntPointCount++];

    pt->x = x;
    pt->y = y;
    pt->speed = speed;
}

// Recursive midpoint subdivision for smooth curves (yyPath.js:225-242)
static void handlePiece(int depth, float x1, float y1, float s1, float x2, float y2, float s2, float x3, float y3, float s3) {
    if (depth == 0) return;

    float mx = (x1 + x2 + x2 + x3) / 4.0f;
    float my = (y1 + y2 + y2 + y3) / 4.0f;
    float ms = (s1 + s2 + s2 + s3) / 4.0f;

    if ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) > 16.0f) {
        handlePiece(depth - 1, x1, y1, s1, (x2 + x1) / 2.0f, (y2 + y1) / 2.0f, (s2 + s1) / 2.0f, mx, my, ms);
    }

    addInternalPoint(mx, my, ms);

    if ((x2 - x3) * (x2 - x3) + (y2 - y3) * (y2 - y3) > 16.0f) {
        handlePiece(depth - 1, mx, my, ms, (x3 + x2) / 2.0f, (y3 + y2) / 2.0f, (s3 + s2) / 2.0f, x3, y3, s3);
    }
}

static void GamePath_computeInternal(GamePath* path) {
    // Reset temp state
    tempIntPoints_free();

    free(path->internalPoints);
    path->internalPoints = NULL;
    path->internalPointCount = 0;
    path->length = 0.0;

    if (path->pointCount == 0)
        return;

    if (path->isSmooth) {
        // ComputeCurved (yyPath.js:254-292)
        if (!path->isClosed) {
            addInternalPoint(path->points[0].x, path->points[0].y, path->points[0].speed);
        }

        int n;
        if (path->isClosed) {
            n = (int) path->pointCount - 1;
        } else {
            n = (int) path->pointCount - 3;
        }

        repeat(n + 1, i) {
            PathPoint* p1 = &path->points[i % path->pointCount];
            PathPoint* p2 = &path->points[(i + 1) % path->pointCount];
            PathPoint* p3 = &path->points[(i + 2) % path->pointCount];
            handlePiece((int) path->precision,
                        (p1->x + p2->x) / 2.0f, (p1->y + p2->y) / 2.0f, (p1->speed + p2->speed) / 2.0f,
                        p2->x, p2->y, p2->speed,
                        (p2->x + p3->x) / 2.0f, (p2->y + p3->y) / 2.0f, (p2->speed + p3->speed) / 2.0f);
        }

        if (!path->isClosed) {
            PathPoint* last = &path->points[path->pointCount - 1];
            addInternalPoint(last->x, last->y, last->speed);
        } else {
            // Closed smooth: append the first internal point again
            addInternalPoint(tempIntPoints[0].x, tempIntPoints[0].y, tempIntPoints[0].speed);
        }
    } else {
        // ComputeLinear (yyPath.js:192-204)
        repeat(path->pointCount, i) {
            addInternalPoint(path->points[i].x, path->points[i].y, path->points[i].speed);
        }
        if (path->isClosed) {
            addInternalPoint(path->points[0].x, path->points[0].y, path->points[0].speed);
        }
    }

    // ComputeLength (yyPath.js:150-160)
    path->internalPointCount = tempIntPointCount;
    path->internalPoints = (InternalPathPoint *)safeMalloc(tempIntPointCount * sizeof(InternalPathPoint));
    memcpy(path->internalPoints, tempIntPoints, tempIntPointCount * sizeof(InternalPathPoint));
    tempIntPoints_free();
    tempIntPoints = NULL;
    tempIntPointCount = 0;

    path->length = 0.0;
    if (path->internalPointCount > 0) {
        path->internalPoints[0].l = 0.0;
        repeat(path->internalPointCount - 1, j) {
            uint32_t i = j + 1;
            float dx = path->internalPoints[i].x - path->internalPoints[i - 1].x;
            float dy = path->internalPoints[i].y - path->internalPoints[i - 1].y;
            path->length += sqrtf(dx * dx + dy * dy);
            path->internalPoints[i].l = path->length;
        }
    }
}

// Get interpolated position at t in [0,1] (yyPath.js:362-409)
static PathPositionResult GamePath_getPosition(GamePath* path, float t) {
    PathPositionResult result = {0};
    result.speed = 100.0f;

    if (path->internalPointCount == 0) return result;

    if (path->internalPointCount == 1 || path->length == 0.0f || 0.0f >= t) {
        result.x = path->internalPoints[0].x;
        result.y = path->internalPoints[0].y;
        result.speed = path->internalPoints[0].speed;
        return result;
    }

    if (t >= 1.0f) {
        InternalPathPoint* last = &path->internalPoints[path->internalPointCount - 1];
        result.x = last->x;
        result.y = last->y;
        result.speed = last->speed;
        return result;
    }

    // Get the right interval via linear scan
    float l = path->length * t;
    uint32_t pos = 0;
    while (path->internalPointCount - 2 > pos && l >= path->internalPoints[pos + 1].l) {
        pos++;
    }

    InternalPathPoint* node = &path->internalPoints[pos];
    float lRem = l - node->l;
    float w = path->internalPoints[pos + 1].l - node->l;

    if (w != 0.0f) {
        InternalPathPoint* next = &path->internalPoints[pos + 1];
        result.x = node->x + lRem * (next->x - node->x) / w;
        result.y = node->y + lRem * (next->y - node->y) / w;
        result.speed = node->speed + lRem * (next->speed - node->speed) / w;
    } else {
        result.x = node->x;
        result.y = node->y;
        result.speed = node->speed;
    }

    return result;
}