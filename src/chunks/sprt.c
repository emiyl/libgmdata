#include "common.h"
#include "../datawin.h"

static int SPRT_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int SPRT_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
static int SPRT_pointerTable_successHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
int SPRT_parse(DataWin *dw) {
    Chunk chunk = {0};
    SprtChunk *s = &dw->sprt;

    if (get_chunk(dw, "SPRT", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "SPRT");

    if (Reader_readAndParsePointerTable(
        &reader, dw,
        (void **)&s->sprites, &s->parsedCount,
        &s->count, sizeof(Sprite), 
        SPRT_pointerTable_parse,
        SPRT_pointerTable_missingHandler,
        SPRT_pointerTable_successHandler
    ) != 0) {
        logWarn("[SPRT_parse] Failed to parse SPRT pointer table\n");
        return -1;
    }

    return 0;
}

static int Sprite_parse(Reader *reader, DataWin *dw, Sprite *spr) {
    if (reader == NULL || dw == NULL || spr == NULL) {
        logWarn("[Sprite_parse] Invalid parameters\n");
        return -1;
    }

    spr->present = true;

    readString(&spr->name, dw);
    read(&spr->width, UInt32);
    read(&spr->height, UInt32);
    read(&spr->marginLeft, Int32);
    read(&spr->marginRight, Int32);
    read(&spr->marginBottom, Int32);
    read(&spr->marginTop, Int32);
    read(&spr->transparent, Bool32);
    read(&spr->smooth, Bool32);
    read(&spr->preload, Bool32);
    read(&spr->bboxMode, UInt32);
    read(&spr->sepMasks, UInt32);
    read(&spr->originX, Int32);
    read(&spr->originY, Int32);

    int32_t check;
    read(&check, Int32);
    uint32_t nineSliceOffset = 0;
    if (check == -1) {
        spr->specialType = true;
        read(&spr->sVersion, UInt32);
        read(&spr->sSpriteType, UInt32);
        if (spr->sSpriteType == 0) {
            // Normal "special" sprite, technically only used for GameMaker: Studio 2+, but some modding tools (like UndertaleModTool) may inject special sprite types,
            // even though the data.win is NOT GM:S 2+
            if (DataWin_isVersionAtLeast(dw, 2, 0, 0, 0)) {
                read(&spr->gms2PlaybackSpeed, Float32);
                read(&spr->gms2PlaybackSpeedType, Bool32);
                if (spr->sVersion >= 2) {
                    Reader_skip(reader, 4); //sequenceOffset;
                    if (spr->sVersion >= 3) {
                        read(&nineSliceOffset, UInt32);
                        if (nineSliceOffset != 0) {
                            nineSliceOffset -= reader->offset; // Convert to relative offset
                        }
                    }
                }
                read(&check, Int32);
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
            read(&spr->tpagIndices[i], Int32);
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

    read(&spr->maskCount, UInt32);
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
        read(&spr->nsLeft, Int32);
        read(&spr->nsTop, Int32);
        read(&spr->nsRight, Int32);
        read(&spr->nsBottom, Int32);
        read(&spr->nineSliceEnabled, Bool32);
        repeat(5, j) {
            int32_t mode;
            read(&mode, Int32);
            spr->nsTileModes[j] = (uint8_t) mode;
        }
        Reader_seek(reader, savedPos);
    }

    return 0;
}

static int SPRT_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)extraData; // Unused parameter
    if (Sprite_parse(reader, dw, (Sprite *)out) != 0) {
        logWarn("[SPRT_pointerTable_parse] Failed to parse Sprite\n");
        return -1;
    };
}

static int SPRT_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[SPRT_pointerTable_missingHandler] Sprite pointer is missing, initializing default values.\n");

    Sprite *spr = (Sprite *)out;
    spr->present = false;
    spr->name = NULL;
    spr->width = 0;
    spr->height = 0;
    spr->marginLeft = 0;
    spr->marginRight = 0;
    spr->marginBottom = 0;
    spr->marginTop = 0;
    spr->transparent = false;
    spr->smooth = false;
    spr->preload = false;
    spr->bboxMode = 0;
    spr->sepMasks = 0;
    spr->originX = 0;
    spr->originY = 0;
    spr->specialType = false;
    spr->sVersion = 0;
    spr->sSpriteType = 0;
    spr->gms2PlaybackSpeed = 1.0f;
    spr->gms2PlaybackSpeedType = false;
    spr->textureCount = 0;
    spr->tpagIndices = NULL;
    spr->maskCount = 0;
    spr->masks = NULL;
    spr->maskWidth = 0;
    spr->maskHeight = 0;
    spr->maskOffsetX = 0;
    spr->maskOffsetY = 0;
    spr->nineSliceEnabled = false;
    spr->nsLeft = 0;
    spr->nsTop = 0;
    spr->nsRight = 0;
    spr->nsBottom = 0;
    repeat(5, j) {
        spr->nsTileModes[j] = 0;
    }

    return 0;
}

static int SPRT_pointerTable_successHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)out;    // Unused parameter

    if (extraData == NULL) {
        logWarn("[SPRT_pointerTable_successHandler] extraData is NULL, cannot increment parsedCount.\n");
        return -1;
    }

    uint32_t *parsedCountPtr = (uint32_t *)(uintptr_t)extraData;
    (*parsedCountPtr)++;

    return 0;
}

static int Sprite_free(Sprite *spr) {
    if (spr == NULL) return -1;

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
    return 0;
}

int SPRT_free(SprtChunk *s) {
    if (s == NULL) return -1;

    int result = 0;
    repeat(s->count, i) {
        if (Sprite_free(&s->sprites[i])) {
            logWarn("[SPRT_free] Failed to free Sprite at index %u\n", i);
            result = -1;
        }
    }
    free(s->sprites);
    s->sprites = NULL;
    s->count = 0;
    
    return result;
}