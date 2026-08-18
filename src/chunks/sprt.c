#include "common.h"
#include "../datawin.h"

int Sprite_parse(Reader *reader, DataWin *dw, Sprite *sprite);

int SPRT_parse(DataWin *dw) {
    Chunk chunk = {0};
    SprtChunk *s = &dw->sprt;

    if (find_chunk(dw, "SPRT", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "SPRT");

    uint32_t count;
    uint32_t *ptrs;
    if (Reader_readPointerTable(&reader, &ptrs, &count) != 0) return -1;
    s->count = count;
    s->parsedCount = 0;

    if (count == 0) {
        s->sprites = NULL;
        free(ptrs);
        return 0;
    }

    s->sprites = (Sprite *)calloc(count, sizeof(Sprite));
    if (s->sprites == NULL) {
        free(ptrs);
        return -1;
    }
    repeat (count, i) {
        if (ptrs[i] == 0 || ptrs[i] >= reader.size) continue;
        Reader_seek(&reader, ptrs[i]);
        if (Sprite_parse(&reader, dw, &s->sprites[i]) != 0) {
            free(ptrs);
            free(s->sprites);
            return -1;
        }
        s->parsedCount++;
    }
    
    free(ptrs);
    return 0;
}

int Sprite_parse(Reader *reader, DataWin *dw, Sprite *spr) {
    if (reader == NULL || dw == NULL || spr == NULL) {
        return -1;
    }

    spr->present = true;
    Reader_readString(reader, dw, &spr->name);
    Reader_readUInt32(reader, &spr->width);
    Reader_readUInt32(reader, &spr->height);
    Reader_readInt32(reader, &spr->marginLeft);
    Reader_readInt32(reader, &spr->marginRight);
    Reader_readInt32(reader, &spr->marginBottom);
    Reader_readInt32(reader, &spr->marginTop);
    Reader_readBool32(reader, &spr->transparent);
    Reader_readBool32(reader, &spr->smooth);
    Reader_readBool32(reader, &spr->preload);
    Reader_readUInt32(reader, &spr->bboxMode);
    Reader_readUInt32(reader, &spr->sepMasks);
    Reader_readInt32(reader, &spr->originX);
    Reader_readInt32(reader, &spr->originY);

    int32_t check;
    Reader_readInt32(reader, &check);
    uint32_t nineSliceOffset = 0;
    if (check == -1) {
        spr->specialType = true;
        Reader_readUInt32(reader, &spr->sVersion);
        Reader_readUInt32(reader, &spr->sSpriteType);
        if (spr->sSpriteType == 0) {
            // Normal "special" sprite, technically only used for GameMaker: Studio 2+, but some modding tools (like UndertaleModTool) may inject special sprite types,
            // even though the data.win is NOT GM:S 2+
            if (DataWin_isVersionAtLeast(dw, 2, 0, 0, 0)) {
                Reader_readFloat32(reader, &spr->gms2PlaybackSpeed);
                Reader_readBool32(reader, &spr->gms2PlaybackSpeedType);
                if (spr->sVersion >= 2) {
                    Reader_skip(reader, 4); //sequenceOffset;
                    if (spr->sVersion >= 3) {
                        Reader_readUInt32(reader, &nineSliceOffset);
                    }
                }
                Reader_readInt32(reader, &check);
            } else {
                // Technically should NEVER happen on legit data.wins
                check = 0;
            }
        } else {
            logWarn("libgmdata: Detected special sprite type %u (%s), but we don't support it yet!\n", spr->sSpriteType, spr->sSpriteType == 2 ? "Spine" : spr->sSpriteType == 1 ? "SWF" : "Unknown");
            spr->textureCount = 0;
            spr->tpagIndices = NULL;
            spr->maskCount = 0;
            spr->masks = NULL;
            return 0;
        }
    }

    // 'check' is the texture count (start of SimpleList)
    spr->textureCount = check;
    if (spr->textureCount > 0) {
        // Temporarily store the absolute file offsets here; parseTPAG resolves them in-place to TPAG indices once the TPAG table is known.
        spr->tpagIndices = (int32_t *)safeMalloc(spr->textureCount * sizeof(int32_t));
        repeat (spr->textureCount, i) {
            Reader_readInt32(reader, &spr->tpagIndices[i]);
        }
    } else {
        spr->tpagIndices = NULL;
    }

    // Collision mask data
    // sepMasks: 0 = axis-aligned rect (no mask data stored in some cases)
    //           1 = precise per-frame masks
    //           2 = rotated rect (no mask data)
    // Mask format: each bit = 1 pixel, MSB first, row-major
    // Width in bytes = (spriteWidth + 7) / 8, total = widthInBytes * spriteHeight
    // After all masks, data is padded to 4-byte alignment
    // Zero-dimension sprites (placeholder/empty assets in test files) omit the mask block entirely
    // GMS 2024.6+ stores collision masks at bounding-box dimensions (marginRight-marginLeft+1 by marginBottom-marginTop+1) instead of the full sprite size.
    // Pre-2024.6 they cover the full sprite.
    if (DataWin_isVersionAtLeast(dw, 2024, 6, 0, 0)) {
        spr->maskWidth = (uint32_t) (spr->marginRight - spr->marginLeft + 1);
        spr->maskHeight = (uint32_t) (spr->marginBottom - spr->marginTop + 1);
        spr->maskOffsetX = spr->marginLeft;
        spr->maskOffsetY = spr->marginTop;
    } else {
        spr->maskWidth = spr->width;
        spr->maskHeight = spr->height;
        spr->maskOffsetX = 0;
        spr->maskOffsetY = 0;
    }

    if (spr->width == 0 || spr->height == 0) {
        spr->maskCount = 0;
        spr->masks = NULL;
        return 0;
    }

    Reader_readUInt32(reader, &spr->maskCount);
    uint32_t maskDataCount = spr->maskCount;
    bool skipLoadingPreciseMasksForNonPreciseSprites = false;

    if (maskDataCount > 0 && spr->maskWidth > 0 && spr->maskHeight > 0) {
        uint32_t bytesPerRow = (spr->maskWidth + 7) / 8;
        uint32_t bytesPerMask = bytesPerRow * spr->maskHeight;

        if (spr->sepMasks == 1 || !skipLoadingPreciseMasksForNonPreciseSprites) {
            spr->masks = (uint8_t **)safeMalloc(maskDataCount * sizeof(uint8_t*));
            spr->maskDataOwned = !dw->mappedFile;
            if (dw->mappedFile) {
                repeat(maskDataCount, j) {
                    spr->masks[j] = dw->mappedFile + reader->cursor;
                    reader->cursor += bytesPerMask;
                }
            } else {
                repeat(maskDataCount, j) {
                    spr->masks[j] = (uint8_t *)safeMalloc(bytesPerMask);
                    Reader_readBytes(reader, spr->masks[j], bytesPerMask);
                }
            }
        } else {
            Reader_skip(reader, bytesPerMask * maskDataCount);
            spr->masks = NULL;
            spr->maskDataOwned = false;
        }
        // Pad the TOTAL mask data to 4-byte alignment (not per-mask)
        uint32_t totalMaskBytes = bytesPerMask * maskDataCount;
        uint32_t remainder = totalMaskBytes % 4;
        if (remainder != 0) {
            Reader_skip(reader, 4 - remainder);
        }
    } else {
        spr->masks = NULL;
    }

    // Nine-slice block (40 bytes). Located at nineSliceOffset (absolute file offset) elsewhere in the chunk.
    if (nineSliceOffset != 0) {
        size_t savedPos = reader->cursor;
        Reader_seek(reader, (size_t) nineSliceOffset);
        Reader_readInt32(reader, &spr->nsLeft);
        Reader_readInt32(reader, &spr->nsTop);
        Reader_readInt32(reader, &spr->nsRight);
        Reader_readInt32(reader, &spr->nsBottom);
        Reader_readBool32(reader, &spr->nineSliceEnabled);
        repeat(5, j) {
            int32_t mode;
            Reader_readInt32(reader, &mode);
            spr->nsTileModes[j] = (uint8_t) mode;
        }
        Reader_seek(reader, savedPos);
    }

    return 0;
}

void Sprite_free(Sprite *spr) {
    if (spr == NULL) return;

    free((void *)spr->name);
    spr->name = NULL;

    free(spr->tpagIndices);
    spr->tpagIndices = NULL;

    if (spr->masks != NULL) {
        if (spr->maskDataOwned) {
            repeat(spr->maskCount, i) {
                free(spr->masks[i]);
            }
        }
        free(spr->masks);
        spr->masks = NULL;
    }
    spr->maskCount = 0;
    spr->maskDataOwned = false;
}

void SPRT_free(SprtChunk *s) {
    if (s == NULL) return;

    repeat(s->count, i) {
        Sprite_free(&s->sprites[i]);
    }
    free(s->sprites);
}