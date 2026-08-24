#include "common.h"

static int BGND_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int BGND_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
int BGND_parse(DataWin *dw) {
    Chunk chunk = {0};
    BgndChunk *b = &dw->bgnd;

    if (get_chunk(dw, "BGND", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "BGND");

    uint32_t *ptrs;
    if (Reader_readPointerTable(&reader, &ptrs, &b->count) != 0) return -1;

    if (b->count == 0) {
        b->backgrounds = NULL;
        free(ptrs);
        return 0;
    }
    
    int result = Reader_parsePointerTable(
        &reader, dw,
        ptrs, b->count,
        (void **)&b->backgrounds, sizeof(Background),
        NULL,
        BGND_pointerTable_parse,
        BGND_pointerTable_missingHandler,
        NULL
    );
    
    free(ptrs);
    return result;
}

static int Background_parse(Reader *reader, DataWin *dw, Background *bg) {
    bg->present = true;
    readString(&bg->name, dw);
    read(&bg->smooth, Bool32);
    read(&bg->preload, Bool32);

    // Temporarily store the absolute file offset; parseTPAG resolves it in-place to a TPAG index once the TPAG table is known.
    read(&bg->tpagIndex, Int32);
    if (DataWin_isVersionAtLeast(dw, 2, 0, 0, 0)) {
        read(&bg->gms2UnknownAlways2, UInt32);
        read(&bg->gms2TileWidth, UInt32);
        read(&bg->gms2TileHeight, UInt32);
        if (DataWin_isVersionAtLeast(dw, 2024, 14, 1, 0)) {
            read(&bg->gms2TileSeparationX, UInt32);
            read(&bg->gms2TileSeparationY, UInt32);
        }
        read(&bg->gms2OutputBorderX, UInt32);
        read(&bg->gms2OutputBorderY, UInt32);
        read(&bg->gms2TileColumns, UInt32);
        read(&bg->gms2ItemsPerTileCount, UInt32);
        read(&bg->gms2TileCount, UInt32);
        read(&bg->gms2ExportedSpriteIndex, Int32);
        read(&bg->gms2FrameLength, Int64);
        int tileIdCount = bg->gms2TileCount * bg->gms2ItemsPerTileCount;
        bg->gms2TileIds = (uint32_t *)safeMalloc(tileIdCount*sizeof(uint32_t));
        repeat(tileIdCount, j) {
            read(&bg->gms2TileIds[j], UInt32);
        }
    }
    
    return 0;
}

static int BGND_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)extraData; // Unused parameter
    return Background_parse(reader, dw, out);
}

static int BGND_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[BGND_pointerTable_missingHandler] Background pointer is missing, initializing default values.\n");

    Background *bg = (Background *)out;
    bg->present = false;
    bg->name = NULL;
    bg->smooth = false;
    bg->preload = false;
    bg->tpagIndex = -1;
    bg->gms2UnknownAlways2 = 0;
    bg->gms2TileWidth = 0;
    bg->gms2TileHeight = 0;
    bg->gms2TileSeparationX = 0;
    bg->gms2TileSeparationY = 0;
    bg->gms2OutputBorderX = 0;
    bg->gms2OutputBorderY = 0;
    bg->gms2TileColumns = 0;
    bg->gms2ItemsPerTileCount = 0;
    bg->gms2TileCount = 0;
    bg->gms2ExportedSpriteIndex = -1;
    bg->gms2FrameLength = 0;
    bg->gms2TileIds = NULL;

    return 0;
}

static int Background_free(Background *bg) {
    if (bg->name) free((void*)bg->name);
    if (bg->gms2TileIds) free(bg->gms2TileIds);
    return 0;
}

int BGND_free(BgndChunk *b) {
    if (b->backgrounds) {
        repeat(b->count, i) {
            Background_free(&b->backgrounds[i]);
        }
        free(b->backgrounds);
        b->backgrounds = NULL;
    }
    b->count = 0;
    return 0;
}