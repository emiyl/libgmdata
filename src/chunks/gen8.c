#include "common.h"

static void GEN8_ParseRoomOrder(Reader *reader, Gen8 *g) {
    Reader_readUInt32(reader, &g->roomOrderCount);

    if (g->roomOrderCount == 0) {
        g->roomOrder = NULL;
        return;
    }

    g->roomOrder = safeMalloc(g->roomOrderCount * sizeof(*g->roomOrder));

    repeat(g->roomOrderCount, i) {
        Reader_readInt32(reader, &g->roomOrder[i]);
    }
}

int GEN8_parse(DataWin *dw) {
    Chunk chunk = {0};
    Gen8* g = &dw->gen8;

    if(find_chunk(dw, "GEN8", &chunk) != 0) return -1;
    if(chunk.offset + chunk.length > dw->file_size) return -1;

    const bool isCompactWad8 = g->wadVersion < 8 && chunk.length <= 108;
    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "GEN8");

    // isDebuggerDisabled and wadVersion
    Reader_readUInt8(&reader, &g->isDebuggerDisabled);
    Reader_readUInt8(&reader, &g->wadVersion);
    Reader_skip(&reader, 2); // padding

    Reader_readString(&reader, dw, &g->fileName);

    if (isCompactWad8) {
        g->config = "";
    } else {
        Reader_readString(&reader, dw, &g->config);
    }

    Reader_readUInt32(&reader, &g->lastObj);
    Reader_readUInt32(&reader, &g->lastTile);
    Reader_readUInt32(&reader, &g->gameID);

    Reader_readBytes(&reader, g->directPlayGuid, sizeof(g->directPlayGuid));

    if (isCompactWad8) {
        // Compact WAD8 has no name/version fields.
        g->name = "";
        g->major = 1;
        g->minor = 0;
        g->release = 0;
        g->build = 198;
    } else {
        Reader_readString(&reader, dw, &g->name);
        Reader_readUInt32(&reader, &g->major);
        Reader_readUInt32(&reader, &g->minor);
        Reader_readUInt32(&reader, &g->release);
        Reader_readUInt32(&reader, &g->build);
    }

    Reader_readUInt32(&reader, &g->defaultWindowWidth);
    Reader_readUInt32(&reader, &g->defaultWindowHeight);
    Reader_readUInt32(&reader, &g->info);
    Reader_readUInt32(&reader, &g->licenseCRC32);
    Reader_readBytes(&reader, g->licenseMD5, sizeof(g->licenseMD5));

    // Compact WAD8 has a slightly different tail
    if (isCompactWad8) {
        uint32_t timestamp;
        Reader_readUInt32(&reader, &timestamp);
        g->timestamp = (uint64_t)timestamp;

        Reader_skip(&reader, 4); // gap at offset 72

        g->displayName = "";
        g->activeTargets = 0;
        g->functionClassifications = 0;
        g->steamAppID = 0;
        g->debuggerPort = 0;

        GEN8_ParseRoomOrder(&reader, g);
        return 0;
    }

    // WAD8 < 12
    if (g->wadVersion < 12) {
        int32_t timestamp;
        Reader_readInt32(&reader, &timestamp);
        g->timestamp = (uint64_t)(int64_t)timestamp;

        Reader_skip(&reader, 4); // padding at body + 0x60

        if (g->wadVersion >= 9) {
            Reader_readString(&reader, dw, &g->displayName);
        } else {
            g->displayName = "";
        }

        if (g->wadVersion >= 11) {
            Reader_readUInt64(&reader, &g->activeTargets);
        } else {
            g->activeTargets = 0;
        }

        if (g->wadVersion >= 12) {
            Reader_readUInt64(&reader, &g->functionClassifications);
        } else {
            g->functionClassifications = 0;
        }

        g->steamAppID = 0;
        g->debuggerPort = 0;

        GEN8_ParseRoomOrder(&reader, g);
        return 0;
    }

    // WAD8 >= 12
    Reader_readUInt64(&reader, &g->timestamp);
    Reader_readString(&reader, dw, &g->displayName);
    Reader_readUInt64(&reader, &g->activeTargets);
    Reader_readUInt64(&reader, &g->functionClassifications);
    Reader_readInt32(&reader, &g->steamAppID);

    if (g->wadVersion >= 14) {
        Reader_readUInt32(&reader, &g->debuggerPort);
    } else {
        g->debuggerPort = 0;
    }

    GEN8_ParseRoomOrder(&reader, g);

    // GMS2+ fields
    if (g->major >= 2) {
        Reader_skip(&reader, 8);      // firstRandom
        Reader_skip(&reader, 8 * 4);  // four random entries

        Reader_readFloat32(&reader, &g->gms2FPS);

        Reader_skip(&reader, 4);      // AllowStatistics
        Reader_skip(&reader, 16);     // GameGUID
    }

    return 0;
}

void GEN8_free(Gen8 *g) {
    if (g == NULL) return;

    free((void *)g->fileName);
    free((void *)g->config);
    free((void *)g->name);
    free((void *)g->displayName);
    free(g->roomOrder);

    g->fileName = NULL;
    g->config = NULL;
    g->name = NULL;
    g->displayName = NULL;
    g->roomOrder = NULL;
}