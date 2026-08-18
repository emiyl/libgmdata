#include "common.h"
#include "../datawin.h"

int Background_parse(Reader *reader, DataWin *dw, Background *bg);

int BGND_parse(DataWin *dw) {
    Chunk chunk = {0};
    Bgnd *b = &dw->bgnd;

    if (find_chunk(dw, "BGND", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, "BGND");

    uint32_t count;
    uint32_t *ptrs;
    if (Reader_readPointerTable(&reader, &ptrs, &count) != 0) return -1;
    b->count = count;

    if (count == 0) {
        b->backgrounds = NULL;
        free(ptrs);
        return 0;
    }

    // GM 2024.14.1 added tile separation parameters for each background
    // To detect it, we'll check if the background's end position is at the chunks end position (if there's only one background) or the start of the next background
    // If it isn't at either of those, then that means it is 2024.14.1+
    if (DataWin_isVersionAtLeast(dw, 2024, 13, 0, 0) && !DataWin_isVersionAtLeast(dw, 2024, 14, 1, 0)) {
        repeat(count, i) {
            if (ptrs[i] == 0) continue;

            // Skip to where the item per tile count + tile count should be in pre-2024.14.1 versions
            Reader_seek(&reader, ptrs[i] + (11 * 4) - chunk.offset);
            uint32_t itemsPerTileCount, tileCount;
            Reader_readUInt32(&reader, &itemsPerTileCount);
            Reader_readUInt32(&reader, &tileCount);

            // Get what might be the end position to compare it with the actual end position
            size_t tpos = ptrs[i] + (16 * 4) + (itemsPerTileCount * tileCount * 4);
            if (count >= 2 && i < count - 1) {
                // Next thing at end position is a background

                // Align to 8 bytes
                if ((tpos % 8) != 0) tpos += 8 - (tpos % 8);

                if (tpos != ptrs[i + 1]) {
                    DataWin_bumpVersionTo(dw, 2024, 14, 1, 0);
                    break;
                }
            }
            else {
                // Next thing at end position is the end of the chunk

                // Align to 16 bytes
                if ((tpos % 16) != 0) tpos += 16 - (tpos % 16);

                if (tpos != chunk.offset + chunk.length) {
                    DataWin_bumpVersionTo(dw, 2024, 14, 1, 0);
                    break;
                }
            }
        }
    }

    b->backgrounds = (Background *)safeCalloc(count, sizeof(Background));
    repeat(count, i) {
        if (ptrs[i] == 0) continue;
        Reader_seek(&reader, ptrs[i] - chunk.offset);
        if (Background_parse(&reader, dw, &b->backgrounds[i]) != 0) {
            free(ptrs);
            return -1;
        }
    }
    
    free(ptrs);
    return 0;
}

int Background_parse(Reader *reader, DataWin *dw, Background *bg) {
    bg->present = true;
    Reader_readString(reader, dw, &bg->name);
    Reader_readBool32(reader, &bg->smooth);
    Reader_readBool32(reader, &bg->preload);

    // Temporarily store the absolute file offset; parseTPAG resolves it in-place to a TPAG index once the TPAG table is known.
    Reader_readInt32(reader, &bg->tpagIndex);
    if (DataWin_isVersionAtLeast(dw, 2, 0, 0, 0)) {
        Reader_readUInt32(reader, &bg->gms2UnknownAlways2);
        Reader_readUInt32(reader, &bg->gms2TileWidth);
        Reader_readUInt32(reader, &bg->gms2TileHeight);
        if (DataWin_isVersionAtLeast(dw, 2024, 14, 1, 0)) {
            Reader_readUInt32(reader, &bg->gms2TileSeparationX);
            Reader_readUInt32(reader, &bg->gms2TileSeparationY);
        }
        Reader_readUInt32(reader, &bg->gms2OutputBorderX);
        Reader_readUInt32(reader, &bg->gms2OutputBorderY);
        Reader_readUInt32(reader, &bg->gms2TileColumns);
        Reader_readUInt32(reader, &bg->gms2ItemsPerTileCount);
        Reader_readUInt32(reader, &bg->gms2TileCount);
        Reader_readInt32(reader, &bg->gms2ExportedSpriteIndex);
        Reader_readInt64(reader, &bg->gms2FrameLength);
        int tileIdCount = bg->gms2TileCount * bg->gms2ItemsPerTileCount;
        bg->gms2TileIds = (uint32_t *)safeMalloc(tileIdCount*sizeof(uint32_t));
        repeat(tileIdCount, j) {
            Reader_readUInt32(reader, &bg->gms2TileIds[j]);
        }
    }

    return 0;
}

int Background_free(Background *bg) {
    if (bg->name) free((void*)bg->name);
    if (bg->gms2TileIds) free(bg->gms2TileIds);
    return 0;
}

int BGND_free(Bgnd *b) {
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