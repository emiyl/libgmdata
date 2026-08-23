#include "common.h"

static int TGIN_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData);
static int TGIN_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData);
static int TGIN_simpleList_parse(Reader *reader, DataWin *dw, int32_t **out_items, uint32_t *out_count);
static void TGIN_group_free(TextureGroupInfo *group);

static int TGIN_simpleList_parse(Reader *reader, DataWin *dw, int32_t **out_items, uint32_t *out_count) {
    uint32_t count = 0;
    read(&count, UInt32);

    if (count == 0) {
        *out_items = NULL;
        *out_count = 0;
        return 0;
    }

    int32_t *items = safeMalloc(count * sizeof(int32_t));
    for (uint32_t i = 0; i < count; ++i) {
        if (Reader_readInt32(reader, &items[i]) != 0) {
            free(items);
            logError("[TGIN_simpleList_parse] Failed to read item %u of %u\n", i, count);
            return -1;
        }
    }

    *out_items = items;
    *out_count = count;
    (void)dw;
    return 0;
}

static int TGIN_group_parse(Reader *reader, DataWin *dw, TextureGroupInfo *group) {
    memset(group, 0, sizeof(*group));
    group->present = true;

    readString(&group->name, dw);

    if (DataWin_isVersionAtLeast(dw, 2022, 9, 0, 0)) {
        readString(&group->directory, dw);
        readString(&group->extension, dw);
        read(&group->loadType, Int32);
    } else {
        group->directory = NULL;
        group->extension = NULL;
        group->loadType = 0;
    }

    uint32_t texturePagesPtr = 0;
    uint32_t spritesPtr = 0;
    uint32_t spineSpritesPtr = 0;
    uint32_t fontsPtr = 0;
    uint32_t tilesetsPtr = 0;

    read(&texturePagesPtr, UInt32);
    read(&spritesPtr, UInt32);
    if (!DataWin_isVersionAtLeast(dw, 2023, 1, 0, 0)) {
        read(&spineSpritesPtr, UInt32);
    }
    read(&fontsPtr, UInt32);
    read(&tilesetsPtr, UInt32);

    const uint32_t baseOffset = reader->offset;
    #define TGIN_REL_PTR(ptr) ((ptr) == 0U ? 0U : ((ptr) >= baseOffset ? ((ptr) - baseOffset) : (ptr)))

    if (texturePagesPtr != 0) {
        uint32_t relativePtr = TGIN_REL_PTR(texturePagesPtr);
        if (Reader_seek(reader, relativePtr) != 0) {
            return -1;
        }
        if (TGIN_simpleList_parse(reader, dw, &group->texturePages, &group->texturePageCount) != 0) {
            return -1;
        }
    }

    if (spritesPtr != 0) {
        uint32_t relativePtr = TGIN_REL_PTR(spritesPtr);
        if (Reader_seek(reader, relativePtr) != 0) {
            return -1;
        }
        if (TGIN_simpleList_parse(reader, dw, &group->sprites, &group->spriteCount) != 0) {
            return -1;
        }
    }

    if (spineSpritesPtr != 0) {
        uint32_t relativePtr = TGIN_REL_PTR(spineSpritesPtr);
        if (Reader_seek(reader, relativePtr) != 0) {
            return -1;
        }
        if (TGIN_simpleList_parse(reader, dw, &group->spineSprites, &group->spineSpriteCount) != 0) {
            return -1;
        }
    }

    if (fontsPtr != 0) {
        uint32_t relativePtr = TGIN_REL_PTR(fontsPtr);
        if (Reader_seek(reader, relativePtr) != 0) {
            return -1;
        }
        if (TGIN_simpleList_parse(reader, dw, &group->fonts, &group->fontCount) != 0) {
            return -1;
        }
    }

    if (tilesetsPtr != 0) {
        uint32_t relativePtr = TGIN_REL_PTR(tilesetsPtr);
        if (Reader_seek(reader, relativePtr) != 0) {
            return -1;
        }
        if (TGIN_simpleList_parse(reader, dw, &group->tilesets, &group->tileSetCount) != 0) {
            return -1;
        }
    }

    #undef TGIN_REL_PTR
    return 0;
}

int TGIN_parse(DataWin *dw) {
    Chunk chunk = {0};
    TginChunk *t = &dw->tgin;

    if (get_chunk(dw, "TGIN", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "TGIN");

    uint32_t version = 0;
    read(&version, UInt32);
    if (version != 1U) {
        logWarn("[TGIN_parse] Unexpected TGIN version: %u\n", version);
    }

    return Reader_readAndParsePointerTable(
        reader, dw,
        (void **)&t->groups, NULL,
        &t->count, sizeof(TextureGroupInfo),
        TGIN_pointerTable_parse,
        TGIN_pointerTable_missingHandler,
        NULL
    );
}

static int TGIN_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)extraData;
    return TGIN_group_parse(reader, dw, (TextureGroupInfo *)out);
}

static int TGIN_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)reader; (void)dw; (void)extraData;
    TextureGroupInfo *group = (TextureGroupInfo *)out;
    memset(group, 0, sizeof(*group));
    group->present = false;
    return 0;
}

static void TGIN_group_free(TextureGroupInfo *group) {
    if (group == NULL) return;

    free((void *)group->name);
    free((void *)group->directory);
    free((void *)group->extension);

    free(group->texturePages);
    free(group->sprites);
    free(group->spineSprites);
    free(group->fonts);
    free(group->tilesets);

    memset(group, 0, sizeof(*group));
}

int TGIN_free(TginChunk *t) {
    if (t == NULL) return -1;

    if (t->groups != NULL) {
        for (uint32_t i = 0; i < t->count; ++i) {
            TGIN_group_free(&t->groups[i]);
        }
        free(t->groups);
    }

    t->groups = NULL;
    t->count = 0;
    return 0;
}
