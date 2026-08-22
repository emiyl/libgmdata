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

    Reader_readString(reader, dw, &room->name);
    Reader_readString(reader, dw, &room->caption);
    Reader_readUInt32(reader, &room->width);
    Reader_readUInt32(reader, &room->height);
    Reader_readUInt32(reader, &room->speed);
    Reader_readBool32(reader, &room->persistent);
    Reader_readUInt32(reader, &room->backgroundColor);
    Reader_readBool32(reader, &room->drawBackgroundColor);
    Reader_readInt32(reader, &room->creationCodeId);
    Reader_readUInt32(reader, &room->flags);
    Reader_readUInt32(reader, &room->backgroundsFileOffset);
    room->backgroundsFileOffset = room->backgroundsFileOffset >= reader->offset ? room->backgroundsFileOffset - reader->offset : 0;
    Reader_readUInt32(reader, &room->viewsFileOffset);
    room->viewsFileOffset = room->viewsFileOffset >= reader->offset ? room->viewsFileOffset - reader->offset : 0;
    Reader_readUInt32(reader, &room->gameObjectsFileOffset);
    room->gameObjectsFileOffset = room->gameObjectsFileOffset >= reader->offset ? room->gameObjectsFileOffset - reader->offset : 0;
    Reader_readUInt32(reader, &room->tilesFileOffset);
    room->tilesFileOffset = room->tilesFileOffset >= reader->offset ? room->tilesFileOffset - reader->offset : 0;
    Reader_readBool32(reader, &room->world);
    Reader_readUInt32(reader, &room->top);
    Reader_readUInt32(reader, &room->left);
    Reader_readUInt32(reader, &room->right);
    Reader_readUInt32(reader, &room->bottom);
    Reader_readFloat32(reader, &room->gravityX);
    Reader_readFloat32(reader, &room->gravityY);
    Reader_readFloat32(reader, &room->metersPerPixel);

    if (DataWin_isVersionAtLeast(dw, 2024, 13, 0, 0)) {
        uint32_t ignored = 0;
        Reader_readUInt32(reader, &ignored);
    }

    room->layersFileOffset = 0;
    if (DataWin_isVersionAtLeast(dw, 2, 0, 0, 0)) {
        Reader_readUInt32(reader, &room->layersFileOffset);
        room->layersFileOffset = room->layersFileOffset >= reader->offset ? room->layersFileOffset - reader->offset : 0;
        if (DataWin_isVersionAtLeast(dw, 2, 3, 0, 0)) {
            uint32_t ignored = 0;
            Reader_readUInt32(reader, &ignored);
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

        Reader_readBool32(reader, &bg->enabled);
        Reader_readBool32(reader, &bg->foreground);
        Reader_readInt32(reader, &bg->backgroundDefinition);
        Reader_readInt32(reader, &bg->x);
        Reader_readInt32(reader, &bg->y);
        Reader_readInt32(reader, &bg->tileX);
        Reader_readInt32(reader, &bg->tileY);
        Reader_readInt32(reader, &bg->speedX);
        Reader_readInt32(reader, &bg->speedY);
        Reader_readBool32(reader, &bg->stretch);
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
        Reader_readBool32(reader, &view->enabled);
        Reader_readInt32(reader, &view->viewX);
        Reader_readInt32(reader, &view->viewY);
        Reader_readInt32(reader, &view->viewWidth);
        Reader_readInt32(reader, &view->viewHeight);
        Reader_readInt32(reader, &view->portX);
        Reader_readInt32(reader, &view->portY);
        Reader_readInt32(reader, &view->portWidth);
        Reader_readInt32(reader, &view->portHeight);
        Reader_readUInt32(reader, &view->borderX);
        Reader_readUInt32(reader, &view->borderY);
        Reader_readInt32(reader, &view->speedX);
        Reader_readInt32(reader, &view->speedY);
        Reader_readInt32(reader, &view->objectId);
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
        Reader_readInt32(reader, &go->x);
        Reader_readInt32(reader, &go->y);
        Reader_readInt32(reader, &go->objectDefinition);
        Reader_readUInt32(reader, &go->instanceID);
        Reader_readInt32(reader, &go->creationCode);
        Reader_readFloat32(reader, &go->scaleX);
        Reader_readFloat32(reader, &go->scaleY);
        if (DataWin_isVersionAtLeast(dw, 2, 2, 2, 302)) {
            Reader_readFloat32(reader, &go->imageSpeed);
            Reader_readInt32(reader, &go->imageIndex);
        } else {
            go->imageSpeed = 1.0f;
            go->imageIndex = 0;
        }
        Reader_readUInt32(reader, &go->color);
        Reader_readFloat32(reader, &go->rotation);
        if (dw->gen8.wadVersion >= 16) {
            Reader_readInt32(reader, &go->preCreateCode);
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
        Reader_readInt32(reader, &tile->x);
        Reader_readInt32(reader, &tile->y);
        Reader_readInt32(reader, &tile->backgroundDefinition);
        Reader_readInt32(reader, &tile->sourceX);
        Reader_readInt32(reader, &tile->sourceY);
        Reader_readUInt32(reader, &tile->width);
        Reader_readUInt32(reader, &tile->height);
        Reader_readInt32(reader, &tile->tileDepth);
        Reader_readUInt32(reader, &tile->instanceID);
        Reader_readFloat32(reader, &tile->scaleX);
        Reader_readFloat32(reader, &tile->scaleY);
        Reader_readUInt32(reader, &tile->color);
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

    Reader_readUInt32(reader, &legacyTilesPtr);
    Reader_readUInt32(reader, &spritesPtr);

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
                Reader_readInt32(reader, &tile->x);
                Reader_readInt32(reader, &tile->y);
                Reader_readInt32(reader, &tile->backgroundDefinition);
                Reader_readInt32(reader, &tile->sourceX);
                Reader_readInt32(reader, &tile->sourceY);
                Reader_readUInt32(reader, &tile->width);
                Reader_readUInt32(reader, &tile->height);
                Reader_readInt32(reader, &tile->tileDepth);
                Reader_readUInt32(reader, &tile->instanceID);
                Reader_readFloat32(reader, &tile->scaleX);
                Reader_readFloat32(reader, &tile->scaleY);
                Reader_readUInt32(reader, &tile->color);
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
                Reader_readString(reader, dw, &sprite->name);
                Reader_readInt32(reader, &sprite->spriteIndex);
                Reader_readInt32(reader, &sprite->x);
                Reader_readInt32(reader, &sprite->y);
                Reader_readFloat32(reader, &sprite->scaleX);
                Reader_readFloat32(reader, &sprite->scaleY);
                Reader_readUInt32(reader, &sprite->color);
                Reader_readFloat32(reader, &sprite->animationSpeed);
                Reader_readUInt32(reader, &sprite->animationSpeedType);
                Reader_readFloat32(reader, &sprite->frameIndex);
                Reader_readFloat32(reader, &sprite->rotation);
            }
        }
        free(spritePtrs);
    }

    return 0;
}

static int RoomLayerBackgroundData_parse(Reader *reader, RoomLayerBackgroundData *bg) {
    memset(bg, 0, sizeof(*bg));
    Reader_readBool32(reader, &bg->visible);
    Reader_readBool32(reader, &bg->foreground);
    Reader_readInt32(reader, &bg->spriteIndex);
    Reader_readBool32(reader, &bg->hTiled);
    Reader_readBool32(reader, &bg->vTiled);
    Reader_readBool32(reader, &bg->stretch);
    Reader_readUInt32(reader, &bg->color);
    Reader_readFloat32(reader, &bg->firstFrame);
    Reader_readFloat32(reader, &bg->animSpeed);
    Reader_readUInt32(reader, &bg->animSpeedType);
    return 0;
}

static int RoomLayerInstancesData_parse(Reader *reader, RoomLayerInstancesData *inst) {
    memset(inst, 0, sizeof(*inst));
    Reader_readUInt32(reader, &inst->instanceCount);
    if (inst->instanceCount > 0) {
        inst->instanceIds = (uint32_t *)safeMalloc(inst->instanceCount * sizeof(uint32_t));
        repeat(inst->instanceCount, k) {
            Reader_readUInt32(reader, &inst->instanceIds[k]);
        }
    } else {
        inst->instanceIds = NULL;
    }
    return 0;
}

static int RoomLayerTilesData_parse(Reader *reader, DataWin *dw, RoomLayerTilesData *tiles) {
    memset(tiles, 0, sizeof(*tiles));
    Reader_readInt32(reader, &tiles->backgroundIndex);
    Reader_readUInt32(reader, &tiles->tilesX);
    Reader_readUInt32(reader, &tiles->tilesY);

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
            Reader_readUInt8(reader, &length);
            if (length >= 128) {
                uint32_t runLength = ((uint32_t)(length & 0x7F)) + 1U;
                uint32_t tile = 0;
                Reader_readUInt32(reader, &tile);
                if (runLength > totalTiles - produced) runLength = totalTiles - produced;
                for (uint32_t k = 0; k < runLength; k++) {
                    tiles->tileData[produced + k] = tile;
                }
                produced += runLength;
            } else {
                uint32_t runLength = (uint32_t)length;
                if (runLength > totalTiles - produced) runLength = totalTiles - produced;
                for (uint32_t k = 0; k < runLength; k++) {
                    Reader_readUInt32(reader, &tiles->tileData[produced + k]);
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
            Reader_readUInt8(reader, &padLength);
            Reader_readUInt32(reader, &padTile);
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
            Reader_readUInt32(reader, &tiles->tileData[k]);
        }
    }

    return 0;
}

static int RoomLayer_parse(Reader *reader, DataWin *dw, Room *room, RoomLayer *layer) {
    (void)room;
    memset(layer, 0, sizeof(*layer));
    Reader_readString(reader, dw, &layer->name);
    Reader_readUInt32(reader, &layer->id);
    Reader_readUInt32(reader, &layer->type);
    Reader_readInt32(reader, &layer->depth);
    Reader_readFloat32(reader, &layer->xOffset);
    Reader_readFloat32(reader, &layer->yOffset);
    Reader_readFloat32(reader, &layer->hSpeed);
    Reader_readFloat32(reader, &layer->vSpeed);
    Reader_readBool32(reader, &layer->visible);
    if (DataWin_isVersionAtLeast(dw, 2022, 1, 0, 0)) {
        bool effectEnabled = false;
        uint32_t effectTypeOffset = 0;
        uint32_t effectPropCount = 0;
        Reader_readBool32(reader, &effectEnabled);
        Reader_readUInt32(reader, &effectTypeOffset);
        Reader_readUInt32(reader, &effectPropCount);
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
                Reader_readUInt32(reader, &effectTypeOffset);
                Reader_readUInt32(reader, &propCount);
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
