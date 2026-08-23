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

    // GM 2024.14.1 added tile separation parameters for each background
    // To detect it, we'll check if the background's end position is at the chunks end position (if there's only one background) or the start of the next background
    // If it isn't at either of those, then that means it is 2024.14.1+
    if (DataWin_isVersionAtLeast(dw, 2024, 13, 0, 0) && !DataWin_isVersionAtLeast(dw, 2024, 14, 1, 0)) {
        repeat(b->count, i) {
            if (ptrs[i] == 0) continue;

            // ptrs[] entries are already relative to the start of the BGND chunk payload,
            // not absolute file offsets. Mix them with chunk.offset and you end up with
            // bogus addresses that exceed the file buffer during the version probe.
            Reader_seek(&reader, ptrs[i] + (11 * 4));
            uint32_t itemsPerTileCount, tileCount;
            Reader_readUInt32(&reader, &itemsPerTileCount);
            Reader_readUInt32(&reader, &tileCount);

            // Get what might be the end position to compare it with the actual end position
            size_t tpos = ptrs[i] + (16 * 4) + (itemsPerTileCount * tileCount * 4);
            if (b->count >= 2 && i < b->count - 1) {
                // Next thing at end position is a background: use chunk-local offsets here,
                // because all pointer-table entries are relative to the BGND payload start.
                if ((tpos % 8) != 0) tpos += 8 - (tpos % 8);

                if (tpos != ptrs[i + 1]) {
                    DataWin_bumpVersionTo(dw, 2024, 14, 1, 0);
                    break;
                }
            }
            else {
                // Next thing at end position is the end of the chunk payload, not the absolute file end.
                if ((tpos % 16) != 0) tpos += 16 - (tpos % 16);

                if (tpos != (size_t)reader.size) {
                    DataWin_bumpVersionTo(dw, 2024, 14, 1, 0);
                    break;
                }
            }
        }
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