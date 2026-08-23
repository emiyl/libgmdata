#include "common.h"

static int TGIN_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData);
static int TGIN_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData);
static int TGIN_simpleList_parse(Reader *reader, DataWin *dw, int32_t **out_items, uint32_t *out_count);
static void TGIN_group_free(TextureGroupInfo *group);

static int TGIN_simpleList_parse(Reader *reader, DataWin *dw, int32_t **out_items, uint32_t *out_count) {
    uint32_t count = 0;
    if (Reader_readUInt32(reader, &count) != 0) {
        return -1;
    }

    if (count == 0) {
        *out_items = NULL;
        *out_count = 0;
        return 0;
    }

    int32_t *items = safeMalloc(count * sizeof(int32_t));
    for (uint32_t i = 0; i < count; ++i) {
        if (Reader_readInt32(reader, &items[i]) != 0) {
            free(items);
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

    if (Reader_readUInt32(reader, &texturePagesPtr) != 0) return -1;
    if (Reader_readUInt32(reader, &spritesPtr) != 0) return -1;
    if (!DataWin_isVersionAtLeast(dw, 2023, 1, 0, 0)) {
        if (Reader_readUInt32(reader, &spineSpritesPtr) != 0) return -1;
    }
    if (Reader_readUInt32(reader, &fontsPtr) != 0) return -1;
    if (Reader_readUInt32(reader, &tilesetsPtr) != 0) return -1;

    if (texturePagesPtr != 0) {
        Reader_seek(reader, texturePagesPtr);
        if (TGIN_simpleList_parse(reader, dw, &group->texturePages, &group->texturePageCount) != 0) {
            return -1;
        }
    }

    if (spritesPtr != 0) {
        Reader_seek(reader, spritesPtr);
        if (TGIN_simpleList_parse(reader, dw, &group->sprites, &group->spriteCount) != 0) {
            return -1;
        }
    }

    if (spineSpritesPtr != 0) {
        Reader_seek(reader, spineSpritesPtr);
        if (TGIN_simpleList_parse(reader, dw, &group->spineSprites, &group->spineSpriteCount) != 0) {
            return -1;
        }
    }

    if (fontsPtr != 0) {
        Reader_seek(reader, fontsPtr);
        if (TGIN_simpleList_parse(reader, dw, &group->fonts, &group->fontCount) != 0) {
            return -1;
        }
    }

    if (tilesetsPtr != 0) {
        Reader_seek(reader, tilesetsPtr);
        if (TGIN_simpleList_parse(reader, dw, &group->tilesets, &group->tileSetCount) != 0) {
            return -1;
        }
    }

    return 0;
}

int TGIN_parse(DataWin *dw) {
    Chunk chunk = {0};
    TginChunk *t = &dw->tgin;

    if (get_chunk(dw, "TGIN", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "TGIN");

    uint32_t version = 0;
    if (Reader_readUInt32(&reader, &version) != 0) {
        return -1;
    }
    if (version != 1U) {
        logWarn("[TGIN_parse] Unexpected TGIN version: %u\n", version);
    }

    return Reader_readAndParsePointerTable(
        &reader, dw,
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
