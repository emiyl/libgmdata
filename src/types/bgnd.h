#ifndef BGND_TYPES_H
#define BGND_TYPES_H

#include <stdint.h>

typedef struct {
    bool present;
    const char* name;
    bool transparent;
    bool smooth;
    bool preload;
    int32_t tpagIndex;      // resolved TPAG index, -1 if unresolved
    uint32_t gms2UnknownAlways2;
    uint32_t gms2TileWidth;
    uint32_t gms2TileHeight;
    uint32_t gms2TileSeparationX;
    uint32_t gms2TileSeparationY;
    uint32_t gms2OutputBorderX;
    uint32_t gms2OutputBorderY;
    uint32_t gms2TileColumns;
    uint32_t gms2ItemsPerTileCount;
    uint32_t gms2TileCount;
    int gms2ExportedSpriteIndex;
    int64_t gms2FrameLength;
    uint32_t *gms2TileIds;
} Background;

typedef struct {
    uint32_t count;
    Background* backgrounds;
} BgndChunk;

#endif