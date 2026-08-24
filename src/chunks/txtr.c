#include "common.h"

typedef struct {
    bool hasGeneratedMips;
    bool has2022_3;
    bool has2022_9;
} TextureArgs;

static int TXTR_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)dw;
    Texture *tex = (Texture *)out;
    memset(tex, 0, sizeof(*tex));
    TextureArgs *args = (TextureArgs *)extraData;

    tex->present = true;
    read(&tex->scaled, UInt32);
    if (args->hasGeneratedMips) {
        read(&tex->generatedMips, UInt32);
    } else {
        tex->generatedMips = 0;
    }
    if (args->has2022_3) {
        read(&tex->textureBlockSize, UInt32);
    } else {
        tex->textureBlockSize = 0;
    }
    if (args->has2022_9) {
        read(&tex->textureWidth, Int32);
        read(&tex->textureHeight, Int32);
        read(&tex->indexInGroup, Int32);
    } else {
        tex->textureWidth = 0;
        tex->textureHeight = 0;
        tex->indexInGroup = 0;
    }
    read(&tex->blobOffset, UInt32);
    tex->blobData = NULL;

    return 0;
}

static int TXTR_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)reader; (void)dw; (void)extraData;
    Texture *tex = (Texture *)out;
    memset(tex, 0, sizeof(*tex));
    tex->present = false;
    return 0;
}

int TXTR_parse(DataWin *dw) {
    Chunk chunk = {0};
    TxtrChunk *t = &dw->txtr;

    if (get_chunk(dw, "TXTR", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "TXTR");

    uint32_t *ptrs;
    Reader_readPointerTable(&reader, &ptrs, &t->count);

    if (t->count == 0) {
        free(ptrs);
        t->textures = NULL;
        return -1;
    }

    TextureArgs args = {0};

    // Read metadata entries
    args.hasGeneratedMips = DataWin_isVersionAtLeast(dw, 2, 0, 0, 0);

    // Detect GMS 2022.3+ (TextureBlockSize field) and 2022.9+ (Width/Height/IndexInGroup fields) by probing the distance between the first two entry pointers.
    // Only works when there are at least 2 textures (which is almost always the case for real games).
    // Layouts:
    //   pre-2022.3: scaled+generatedMips+blobOffset = 12 bytes
    //   2022.3+: ... + textureBlockSize = 16 bytes
    //   2022.9+: ... + width + height + indexInGroup = 28 bytes
    args.has2022_3 = DataWin_isVersionAtLeast(dw, 2022, 3, 0, 0);
    args.has2022_9 = DataWin_isVersionAtLeast(dw, 2022, 9, 0, 0);
    if (t->count >= 2 && args.hasGeneratedMips && !args.has2022_9 && ptrs[0] != 0 && ptrs[1] != 0) {
        uint32_t diff = ptrs[1] - ptrs[0];
        if (diff == 28) {
            args.has2022_3 = true;
            args.has2022_9 = true;
        } else if (diff == 16 && !args.has2022_3) {
            args.has2022_3 = true;
        }
    }

    if(Reader_parsePointerTable(
        &reader, dw,
        ptrs, t->count,
        (void**)&t->textures, sizeof(Texture),
        &args,
        TXTR_pointerTable_parse,
        TXTR_pointerTable_missingHandler,
        NULL
    )) {
        free(ptrs);
        logWarn("TXTR: Failed to parse pointer table");
        return -1;
    };

    free(ptrs);

    // Compute blob sizes from successive offsets
    {
    repeat(t->count, i) {
        if (t->textures[i].blobOffset == 0) {
            t->textures[i].blobSize = 0; // external texture
            continue;
        }
        if (t->count > i + 1 && t->textures[i + 1].blobOffset != 0) {
            t->textures[i].blobSize = t->textures[i + 1].blobOffset - t->textures[i].blobOffset;
        } else {
            uint32_t chunkEnd = chunk.offset + chunk.length;
            t->textures[i].blobSize = (uint32_t)(chunkEnd - t->textures[i].blobOffset);
        }
    }
    }

    // Load texture payloads either as raw bytes or decoded RGBA depending on the parser option.
    if (!dw->lazyLoadTextures) {
        repeat(t->count, i) {
            if (t->textures[i].blobOffset == 0 || t->textures[i].blobSize == 0) continue;

            if (dw->decodeTextures) {
                const uint8_t *blob = NULL;
                uint32_t blobSize = t->textures[i].blobSize;
                uint8_t *tempBlob = NULL;

                if (dw->mappedFile) {
                    blob = dw->mappedFile + t->textures[i].blobOffset;
                    t->textures[i].mapped = true;
                } else {
                    uint32_t offset = t->textures[i].blobOffset - chunk.offset;
                    Reader_seek(&reader, offset);
                    tempBlob = (uint8_t *)safeMalloc(blobSize);
                    if (tempBlob == NULL) {
                        logWarn("TXTR: Failed to allocate %u bytes for texture %u\n", blobSize, i);
                        continue;
                    }
                    Reader_readBytes(&reader, tempBlob, blobSize);
                    blob = tempBlob;
                    t->textures[i].mapped = false;
                }

                int decodedW = 0;
                int decodedH = 0;
                uint8_t *decoded = TextureDecode_decodeToRgba(
                    blob,
                    blobSize,
                    DataWin_isVersionAtLeast(dw, 2022, 5, 0, 0),
                    &decodedW,
                    &decodedH
                );

                if (tempBlob != NULL) {
                    free(tempBlob);
                }

                if (decoded == NULL) {
                    logWarn("TXTR: Failed to decode texture %u to RGBA\n", i);
                    continue;
                }

                uint32_t decodedSize = (decodedW > 0 && decodedH > 0)
                    ? (uint32_t)((uint64_t)decodedW * (uint64_t)decodedH * 4ULL)
                    : blobSize;

                t->textures[i].blobData = decoded;
                t->textures[i].blobSize = decodedSize;
                t->textures[i].mapped = false;
            } else if (dw->mappedFile) {
                t->textures[i].blobData = dw->mappedFile + t->textures[i].blobOffset;
                t->textures[i].mapped = true;
            } else {
                uint32_t offset = t->textures[i].blobOffset - chunk.offset;
                Reader_seek(&reader, offset);
                t->textures[i].blobData = (uint8_t *)safeMalloc(t->textures[i].blobSize);
                if (t->textures[i].blobData == NULL) {
                    logWarn("TXTR: Failed to allocate %u bytes for texture %u\n",
                            t->textures[i].blobSize, i);
                    continue;
                }
                t->textures[i].mapped = false;
                Reader_readBytes(&reader, t->textures[i].blobData, t->textures[i].blobSize);
            }
        }
    }

    return 0;
}

int TXTR_free(TxtrChunk *t) {
    if (t == NULL) return -1;
    if (t->textures != NULL) {
        repeat(t->count, i) {
            if (!t->textures[i].mapped && t->textures[i].blobData != NULL) {
                free(t->textures[i].blobData);
                t->textures[i].blobData = NULL;
            }
        }
    }
    free(t->textures);
    t->textures = NULL;
    t->count = 0;
    return 0;
}
