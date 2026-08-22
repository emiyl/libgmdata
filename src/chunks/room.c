#include "common.h"
#include "../datawin.h"

static int ROOM_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData);
static int ROOM_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData);
static int RoomLayer_parse(Reader *reader, DataWin *dw, Room *room, RoomLayer *layer);
static int RoomLayerAssetsData_parse(Reader *reader, DataWin *dw, RoomLayerAssetsData *assets);
static int RoomLayerBackgroundData_parse(Reader *reader, RoomLayerBackgroundData *bg);
static int RoomLayerInstancesData_parse(Reader *reader, RoomLayerInstancesData *inst);
static int RoomLayerTilesData_parse(Reader *reader, DataWin *dw, RoomLayerTilesData *tiles);
static int RoomBackgrounds_parse(Reader *reader, Room *room);
static int RoomViews_parse(Reader *reader, Room *room);
static int RoomGameObjects_parse(Reader *reader, DataWin *dw, Room *room);
static int RoomTiles_parse(Reader *reader, DataWin *dw, Room *room);
static int RoomPayload_parse(Reader *reader, DataWin *dw, Room *room);
static int RoomHeader_parse(Reader *reader, DataWin *dw, Room *room);
static void Room_freeLayer(RoomLayer *layer);
static void Room_freePayload(Room *room);

int ROOM_parse(DataWin *dw) {
    Chunk chunk = {0};
    RoomChunk *r = &dw->room;

    if (get_chunk(dw, "ROOM", &chunk) != 0) {
        r->count = 0;
        r->rooms = NULL;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "ROOM");

    return Reader_readAndParsePointerTable(
        &reader, dw,
        (void **)&r->rooms, NULL,
        &r->count, sizeof(Room),
        ROOM_pointerTable_parse,
        ROOM_pointerTable_missingHandler,
        NULL
    );
}

static int RoomHeader_parse(Reader *reader, DataWin *dw, Room *room) {
    room->present = true;
    room->name = NULL;
    room->caption = NULL;
    room->backgrounds = NULL;
    room->views = NULL;
    room->gameObjects = NULL;
    room->gameObjectCount = 0;
    room->tiles = NULL;
    room->tileCount = 0;
    room->layers = NULL;
    room->layerCount = 0;
    room->payloadLoaded = false;
    room->eagerlyLoaded = false;

    readString(&room->name, dw);
    readString(&room->caption, dw);
    read(&room->width, UInt32);
    read(&room->height, UInt32);
    read(&room->speed, UInt32);
    read(&room->persistent, Bool32);
    read(&room->backgroundColor, UInt32);
    read(&room->drawBackgroundColor, Bool32);
    read(&room->creationCodeId, Int32);
    read(&room->flags, UInt32);
    read(&room->backgroundsFileOffset, UInt32);
    room->backgroundsFileOffset = room->backgroundsFileOffset >= reader->offset ? room->backgroundsFileOffset - reader->offset : 0;
    read(&room->viewsFileOffset, UInt32);
    room->viewsFileOffset = room->viewsFileOffset >= reader->offset ? room->viewsFileOffset - reader->offset : 0;
    read(&room->gameObjectsFileOffset, UInt32);
    room->gameObjectsFileOffset = room->gameObjectsFileOffset >= reader->offset ? room->gameObjectsFileOffset - reader->offset : 0;
    read(&room->tilesFileOffset, UInt32);
    room->tilesFileOffset = room->tilesFileOffset >= reader->offset ? room->tilesFileOffset - reader->offset : 0;
    read(&room->world, Bool32);
    read(&room->top, UInt32);
    read(&room->left, UInt32);
    read(&room->right, UInt32);
    read(&room->bottom, UInt32);
    read(&room->gravityX, Float32);
    read(&room->gravityY, Float32);
    read(&room->metersPerPixel, Float32);

    if (DataWin_isVersionAtLeast(dw, 2024, 13, 0, 0)) {
        uint32_t ignored = 0;
        read(&ignored, UInt32);
    }

    room->layersFileOffset = 0;
    if (DataWin_isVersionAtLeast(dw, 2, 0, 0, 0)) {
        read(&room->layerCount, UInt32);
        room->layersFileOffset = room->layersFileOffset >= reader->offset ? room->layersFileOffset - reader->offset : 0;
        if (DataWin_isVersionAtLeast(dw, 2, 3, 0, 0)) {
            uint32_t ignored = 0;
            read(&ignored, UInt32);
        }
    }

    return RoomPayload_parse(reader, dw, room);
}

static int RoomBackgrounds_parse(Reader *reader, Room *room) {
    uint32_t bgCount = 0;
    uint32_t *bgPtrs = NULL;
    if (Reader_readPointerTable(reader, &bgPtrs, &bgCount) != 0) return -1;

    room->backgrounds = (RoomBackground *)safeMalloc(8 * sizeof(RoomBackground));
    uint32_t fillEnd = bgCount < 8 ? bgCount : 8;
    repeat(fillEnd, j) {
        Reader_seek(reader, bgPtrs[j]);
        RoomBackground *bg = &room->backgrounds[j];
        bg->enabled = false;
        bg->foreground = false;
        bg->backgroundDefinition = 0;
        bg->x = 0;
        bg->y = 0;
        bg->tileX = 0;
        bg->tileY = 0;
        bg->speedX = 0;
        bg->speedY = 0;
        bg->stretch = false;

        read(&bg->enabled, Bool32);
        read(&bg->foreground, Bool32);
        read(&bg->backgroundDefinition, Int32);
        read(&bg->x, Int32);
        read(&bg->y, Int32);
        read(&bg->tileX, Int32);
        read(&bg->tileY, Int32);
        read(&bg->speedX, Int32);
        read(&bg->speedY, Int32);
        read(&bg->stretch, Bool32);
    }

    for (uint32_t j = fillEnd; j < 8; j++) {
        memset(&room->backgrounds[j], 0, sizeof(RoomBackground));
    }

    free(bgPtrs);
    return 0;
}

static int RoomViews_parse(Reader *reader, Room *room) {
    uint32_t viewCount = 0;
    uint32_t *viewPtrs = NULL;
    if (Reader_readPointerTable(reader, &viewPtrs, &viewCount) != 0) return -1;

    room->views = (RoomView *)safeMalloc(8 * sizeof(RoomView));
    repeat(viewCount < 8 ? viewCount : 8, j) {
        Reader_seek(reader, viewPtrs[j]);
        RoomView *view = &room->views[j];
        memset(view, 0, sizeof(*view));

        read(&view->enabled, Bool32);
        read(&view->viewX, Int32);
        read(&view->viewY, Int32);
        read(&view->viewWidth, Int32);
        read(&view->viewHeight, Int32);
        read(&view->portX, Int32);
        read(&view->portY, Int32);
        read(&view->portWidth, Int32);
        read(&view->portHeight, Int32);
        read(&view->borderX, UInt32);
        read(&view->borderY, UInt32);
        read(&view->speedX, Int32);
        read(&view->speedY, Int32);
        read(&view->objectId, Int32);
    }
    for (uint32_t j = viewCount < 8 ? viewCount : 8; j < 8; j++) {
        memset(&room->views[j], 0, sizeof(RoomView));
    }

    free(viewPtrs);
    return 0;
}

static int RoomGameObjects_parse(Reader *reader, DataWin *dw, Room *room) {
    uint32_t objCount = 0;
    uint32_t *objPtrs = NULL;
    if (Reader_readPointerTable(reader, &objPtrs, &objCount) != 0) return -1;

    room->gameObjectCount = objCount;
    if (objCount == 0) {
        room->gameObjects = NULL;
        free(objPtrs);
        return 0;
    }

    room->gameObjects = (RoomGameObject *)safeMalloc(objCount * sizeof(RoomGameObject));
    repeat(objCount, j) {
        Reader_seek(reader, objPtrs[j]);
        RoomGameObject *go = &room->gameObjects[j];
        memset(go, 0, sizeof(*go));

        read(&go->x, Int32);
        read(&go->y, Int32);
        read(&go->objectDefinition, Int32);
        read(&go->instanceID, UInt32);
        read(&go->creationCode, Int32);
        read(&go->scaleX, Float32);
        read(&go->scaleY, Float32);
        if (DataWin_isVersionAtLeast(dw, 2, 2, 2, 302)) {
            read(&go->imageSpeed, Float32);
            read(&go->imageIndex, Int32);
        } else {
            go->imageSpeed = 1.0f;
            go->imageIndex = 0;
        }
        read(&go->color, UInt32);
        read(&go->rotation, Float32);
        if (dw->gen8.wadVersion >= 16) {
            read(&go->preCreateCode, Int32);
        } else {
            go->preCreateCode = -1;
        }
    }

    free(objPtrs);
    return 0;
}

static float RoomTile_alphaFromColor(uint32_t color) {
    uint8_t alphaByte = (uint8_t)((color >> 24) & 0xFF);
    return alphaByte == 0 ? 1.0f : (float)alphaByte / 255.0f;
}

static int RoomTiles_parse(Reader *reader, DataWin *dw, Room *room) {
    uint32_t tileCount = 0;
    uint32_t *tilePtrs = NULL;
    if (Reader_readPointerTable(reader, &tilePtrs, &tileCount) != 0) return -1;

    room->tileCount = tileCount;
    if (tileCount == 0) {
        room->tiles = NULL;
        free(tilePtrs);
        return 0;
    }

    room->tiles = (RoomTile *)safeMalloc(tileCount * sizeof(RoomTile));
    repeat(tileCount, j) {
        Reader_seek(reader, tilePtrs[j]);
        RoomTile *tile = &room->tiles[j];
        memset(tile, 0, sizeof(*tile));
        tile->x = 0;
        tile->y = 0;
        tile->useSpriteDefinition = DataWin_isVersionAtLeast(dw, 2, 0, 0, 0);

        read(&tile->x, Int32);
        read(&tile->y, Int32);
        read(&tile->backgroundDefinition, Int32);
        read(&tile->sourceX, Int32);
        read(&tile->sourceY, Int32);
        read(&tile->width, UInt32);
        read(&tile->height, UInt32);
        read(&tile->tileDepth, Int32);
        read(&tile->instanceID, UInt32);
        read(&tile->scaleX, Float32);
        read(&tile->scaleY, Float32);
        read(&tile->color, UInt32);
        tile->alpha = RoomTile_alphaFromColor(tile->color);
    }

    free(tilePtrs);
    return 0;
}

static int RoomLayerAssetsData_parse(Reader *reader, DataWin *dw, RoomLayerAssetsData *assets) {
    uint32_t legacyTilesPtr = 0;
    uint32_t spritesPtr = 0;
    assets->legacyTiles = NULL;
    assets->legacyTileCount = 0;
    assets->sprites = NULL;
    assets->spriteCount = 0;

    read(&legacyTilesPtr, UInt32);
    read(&spritesPtr, UInt32);

    if (legacyTilesPtr != 0) {
        Reader_seek(reader, legacyTilesPtr);
        uint32_t *innerTilePtrs = NULL;
        uint32_t innerCount = 0;
        if (Reader_readPointerTable(reader, &innerTilePtrs, &innerCount) != 0) return -1;
        assets->legacyTileCount = innerCount;
        if (innerCount > 0) {
            assets->legacyTiles = (RoomTile *)safeMalloc(innerCount * sizeof(RoomTile));
            repeat(innerCount, k) {
                Reader_seek(reader, innerTilePtrs[k]);
                RoomTile *tile = &assets->legacyTiles[k];
                memset(tile, 0, sizeof(*tile));
                tile->useSpriteDefinition = DataWin_isVersionAtLeast(dw, 2, 0, 0, 0);

                read(&tile->x, Int32);
                read(&tile->y, Int32);
                read(&tile->backgroundDefinition, Int32);
                read(&tile->sourceX, Int32);
                read(&tile->sourceY, Int32);
                read(&tile->width, UInt32);
                read(&tile->height, UInt32);
                read(&tile->tileDepth, Int32);
                read(&tile->instanceID, UInt32);
                read(&tile->scaleX, Float32);
                read(&tile->scaleY, Float32);
                read(&tile->color, UInt32);

                tile->alpha = RoomTile_alphaFromColor(tile->color);
            }
        }
        free(innerTilePtrs);
    }

    if (spritesPtr != 0) {
        Reader_seek(reader, spritesPtr);
        uint32_t *spritePtrs = NULL;
        uint32_t spriteCount = 0;
        if (Reader_readPointerTable(reader, &spritePtrs, &spriteCount) != 0) return -1;
        assets->spriteCount = spriteCount;
        if (spriteCount > 0) {
            assets->sprites = (SpriteInstance *)safeMalloc(spriteCount * sizeof(SpriteInstance));
            repeat(spriteCount, k) {
                Reader_seek(reader, spritePtrs[k]);
                SpriteInstance *sprite = &assets->sprites[k];
                memset(sprite, 0, sizeof(*sprite));

                readString(&sprite->name, dw);
                read(&sprite->spriteIndex, Int32);
                read(&sprite->x, Int32);
                read(&sprite->y, Int32);
                read(&sprite->scaleX, Float32);
                read(&sprite->scaleY, Float32);
                read(&sprite->color, UInt32);
                read(&sprite->animationSpeed, Float32);
                read(&sprite->animationSpeedType, UInt32);
                read(&sprite->frameIndex, Float32);
                read(&sprite->rotation, Float32);
            }
        }
        free(spritePtrs);
    }

    return 0;
}

static int RoomLayerBackgroundData_parse(Reader *reader, RoomLayerBackgroundData *bg) {
    memset(bg, 0, sizeof(*bg));

    read(&bg->visible, Bool32);
    read(&bg->foreground, Bool32);
    read(&bg->spriteIndex, Int32);
    read(&bg->hTiled, Bool32);
    read(&bg->vTiled, Bool32);
    read(&bg->stretch, Bool32);
    read(&bg->color, UInt32);
    read(&bg->firstFrame, Float32);
    read(&bg->animSpeed, Float32);
    read(&bg->animSpeedType, UInt32);
    
    return 0;
}

static int RoomLayerInstancesData_parse(Reader *reader, RoomLayerInstancesData *inst) {
    memset(inst, 0, sizeof(*inst));

    read(&inst->instanceCount, UInt32);
    
    if (inst->instanceCount > 0) {
        inst->instanceIds = (uint32_t *)safeMalloc(inst->instanceCount * sizeof(uint32_t));
        repeat(inst->instanceCount, k) {
            read(&inst->instanceIds[k], UInt32);
        }
    } else {
        inst->instanceIds = NULL;
    }
    return 0;
}

static int RoomLayerTilesData_parse(Reader *reader, DataWin *dw, RoomLayerTilesData *tiles) {
    memset(tiles, 0, sizeof(*tiles));
    
    read(&tiles->backgroundIndex, Int32);
    read(&tiles->tilesX, UInt32);
    read(&tiles->tilesY, UInt32);

    uint32_t totalTiles = tiles->tilesX * tiles->tilesY;
    if (totalTiles == 0) {
        tiles->tileData = NULL;
        return 0;
    }

    tiles->tileData = (uint32_t *)safeMalloc(totalTiles * sizeof(uint32_t));
    if (DataWin_isVersionAtLeast(dw, 2024, 2, 0, 0)) {
        uint32_t produced = 0;
        while (produced < totalTiles) {
            uint8_t length = 0;
            read(&length, UInt8);

            if (length >= 128) {
                uint32_t runLength = ((uint32_t)(length & 0x7F)) + 1U;
                uint32_t tile = 0;
                read(&tile, UInt32);

                if (runLength > totalTiles - produced) runLength = totalTiles - produced;
                for (uint32_t k = 0; k < runLength; k++) {
                    tiles->tileData[produced + k] = tile;
                }
                produced += runLength;
            } else {
                uint32_t runLength = (uint32_t)length;
                if (runLength > totalTiles - produced) runLength = totalTiles - produced;
                for (uint32_t k = 0; k < runLength; k++) {
                    read(&tiles->tileData[produced + k], UInt32);
                }
                produced += runLength;
            }
        }

        bool hasPadding = false;
        if (totalTiles == 1) {
            hasPadding = true;
        } else if (totalTiles >= 2) {
            hasPadding = tiles->tileData[totalTiles - 1] != tiles->tileData[totalTiles - 2];
        }
        if (hasPadding) {
            uint8_t padLength = 0;
            uint32_t padTile = 0;
            read(&padLength, UInt8);
            read(&padTile, UInt32);
            (void)padLength;
            (void)padTile;
        }
        if (DataWin_isVersionAtLeast(dw, 2024, 4, 0, 0)) {
            size_t pos = reader->cursor;
            size_t aligned = (pos + 3u) & ~(size_t)3u;
            if (aligned > pos) {
                Reader_skip(reader, (int)(aligned - pos));
            }
        }
    } else {
        repeat(totalTiles, k) {
            read(&tiles->tileData[k], UInt32);
        }
    }

    return 0;
}

static int RoomLayer_parse(Reader *reader, DataWin *dw, Room *room, RoomLayer *layer) {
    (void)room;
    memset(layer, 0, sizeof(*layer));

    readString(&layer->name, dw);
    read(&layer->id, UInt32);
    read(&layer->type, UInt32);
    read(&layer->depth, Int32);
    read(&layer->xOffset, Float32);
    read(&layer->yOffset, Float32);
    read(&layer->hSpeed, Float32);
    read(&layer->vSpeed, Float32);
    read(&layer->visible, Bool32);
    
    if (DataWin_isVersionAtLeast(dw, 2022, 1, 0, 0)) {
        bool effectEnabled = false;
        uint32_t effectTypeOffset = 0;
        uint32_t effectPropCount = 0;
        read(&effectEnabled, Bool32);
        read(&effectTypeOffset, UInt32);
        read(&effectPropCount, UInt32);
        if (effectPropCount > 0) {
            uint32_t skip = effectPropCount * 12u;
            Reader_skip(reader, (int)skip);
        }
        (void)effectEnabled;
        (void)effectTypeOffset;
    }

    switch (layer->type) {
        case RoomLayerType_Path:
        case RoomLayerType_Path2:
            break;
        case RoomLayerType_Effect:
            if (!DataWin_isVersionAtLeast(dw, 2022, 1, 0, 0)) {
                uint32_t effectTypeOffset = 0;
                uint32_t propCount = 0;
                read(&effectTypeOffset, UInt32);
                read(&propCount, UInt32);
                if (propCount > 0) {
                    Reader_skip(reader, (int)(propCount * 12u));
                }
                (void)effectTypeOffset;
            }
            break;
        case RoomLayerType_Assets: {
            RoomLayerAssetsData *assets = (RoomLayerAssetsData *)safeMalloc(sizeof(RoomLayerAssetsData));
            if (RoomLayerAssetsData_parse(reader, dw, assets) != 0) {
                free(assets);
                return -1;
            }
            layer->assetsData = assets;
            break;
        }
        case RoomLayerType_Background: {
            RoomLayerBackgroundData *bg = (RoomLayerBackgroundData *)safeMalloc(sizeof(RoomLayerBackgroundData));
            if (RoomLayerBackgroundData_parse(reader, bg) != 0) {
                free(bg);
                return -1;
            }
            layer->backgroundData = bg;
            break;
        }
        case RoomLayerType_Instances: {
            RoomLayerInstancesData *inst = (RoomLayerInstancesData *)safeMalloc(sizeof(RoomLayerInstancesData));
            if (RoomLayerInstancesData_parse(reader, inst) != 0) {
                free(inst);
                return -1;
            }
            layer->instancesData = inst;
            break;
        }
        case RoomLayerType_Tiles: {
            RoomLayerTilesData *tiles = (RoomLayerTilesData *)safeMalloc(sizeof(RoomLayerTilesData));
            if (RoomLayerTilesData_parse(reader, dw, tiles) != 0) {
                free(tiles);
                return -1;
            }
            layer->tilesData = tiles;
            break;
        }
        default:
            logWarn("[ROOM] Unsupported Room Layer Type %u\n", layer->type);
            return -1;
    }

    return 0;
}

static int RoomPayload_parse(Reader *reader, DataWin *dw, Room *room) {
    if (room->payloadLoaded) return 0;

    if (room->backgroundsFileOffset != 0) {
        Reader_seek(reader, room->backgroundsFileOffset);
        if (RoomBackgrounds_parse(reader, room) != 0) return -1;
    }
    if (room->viewsFileOffset != 0) {
        Reader_seek(reader, room->viewsFileOffset);
        if (RoomViews_parse(reader, room) != 0) return -1;
    }
    if (room->gameObjectsFileOffset != 0) {
        Reader_seek(reader, room->gameObjectsFileOffset);
        if (RoomGameObjects_parse(reader, dw, room) != 0) return -1;
    }
    if (room->tilesFileOffset != 0) {
        Reader_seek(reader, room->tilesFileOffset);
        if (RoomTiles_parse(reader, dw, room) != 0) return -1;
    }
    if (room->layersFileOffset != 0) {
        Reader_seek(reader, room->layersFileOffset);
        uint32_t layerCount = 0;
        uint32_t *layerPtrs = NULL;
        if (Reader_readPointerTable(reader, &layerPtrs, &layerCount) != 0) return -1;
        room->layerCount = layerCount;
        if (layerCount > 0) {
            room->layers = (RoomLayer *)safeMalloc(layerCount * sizeof(RoomLayer));
            repeat(layerCount, j) {
                Reader_seek(reader, layerPtrs[j]);
                if (RoomLayer_parse(reader, dw, room, &room->layers[j]) != 0) {
                    free(layerPtrs);
                    return -1;
                }
            }
        } else {
            room->layers = NULL;
        }
        free(layerPtrs);
    }

    room->payloadLoaded = true;
    return 0;
}

static int ROOM_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)extraData;
    if (reader == NULL || dw == NULL || out == NULL) return -1;

    Room *room = (Room *)out;
    if (RoomHeader_parse(reader, dw, room) != 0) {
        logWarn("[ROOM_pointerTable_parse] Failed parsing room at offset %zu\n", reader->cursor);
        return -1;
    }
    return 0;
}

static int ROOM_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)reader;
    (void)dw;
    (void)extraData;
    Room *room = (Room *)out;
    memset(room, 0, sizeof(*room));
    room->present = false;
    room->creationCodeId = -1;
    return 0;
}

static void Room_freeLayer(RoomLayer *layer) {
    if (layer == NULL) return;
    free((void *)layer->name);
    layer->name = NULL;
    if (layer->assetsData != NULL) {
        free(layer->assetsData->legacyTiles);
        layer->assetsData->legacyTiles = NULL;
        free(layer->assetsData->sprites);
        layer->assetsData->sprites = NULL;
        free(layer->assetsData);
        layer->assetsData = NULL;
    }
    if (layer->backgroundData != NULL) {
        free(layer->backgroundData);
        layer->backgroundData = NULL;
    }
    if (layer->instancesData != NULL) {
        free(layer->instancesData->instanceIds);
        layer->instancesData->instanceIds = NULL;
        free(layer->instancesData);
        layer->instancesData = NULL;
    }
    if (layer->tilesData != NULL) {
        free(layer->tilesData->tileData);
        layer->tilesData->tileData = NULL;
        free(layer->tilesData);
        layer->tilesData = NULL;
    }
}

static void Room_freePayload(Room *room) {
    if (room == NULL) return;
    free(room->backgrounds);
    room->backgrounds = NULL;
    free(room->views);
    room->views = NULL;
    free(room->gameObjects);
    room->gameObjects = NULL;
    room->gameObjectCount = 0;
    free(room->tiles);
    room->tiles = NULL;
    room->tileCount = 0;
    if (room->layers != NULL) {
        repeat(room->layerCount, i) {
            Room_freeLayer(&room->layers[i]);
        }
        free(room->layers);
        room->layers = NULL;
    }
    room->layerCount = 0;
    room->payloadLoaded = false;
}

int ROOM_free(RoomChunk *r) {
    if (r == NULL) return -1;
    repeat(r->count, i) {
        Room *room = &r->rooms[i];
        free((void *)room->name);
        room->name = NULL;
        free((void *)room->caption);
        room->caption = NULL;
        Room_freePayload(room);
    }
    free(r->rooms);
    r->rooms = NULL;
    r->count = 0;
    return 0;
}
