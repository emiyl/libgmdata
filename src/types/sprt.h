#ifndef SPRT_TYPES_H
#define SPRT_TYPES_H

#include <stdint.h>

typedef struct {
    bool present;
    const char* name;
    uint32_t width;
    uint32_t height;
    int32_t marginLeft;
    int32_t marginRight;
    int32_t marginBottom;
    int32_t marginTop;
    bool transparent;
    bool smooth;
    bool preload;
    uint32_t bboxMode;
    uint32_t sepMasks;
    int32_t originX;
    int32_t originY;
    uint32_t sVersion;
    uint32_t sSpriteType;
    float gms2PlaybackSpeed;
    bool gms2PlaybackSpeedType;
    bool specialType;
    uint32_t textureCount;
    int32_t* tpagIndices;    // resolved TPAG indices (one per frame); -1 for unresolved
    uint32_t maskCount;       // number of collision masks (one per frame, or 0)
    uint8_t** masks;          // array of maskCount packed bit arrays (nullptr if none)
    bool maskDataOwned;       // true when masks[] entries were heap-allocated; false when they point into the file mapping
    // Collision mask storage dimensions. Pre-2024.6 these equal the full sprite width/height with zero offset.
    // GMS 2024.6+ stores masks at bounding-box dimensions, so the mask covers only [maskOffsetX, maskOffsetX+maskWidth).
    uint32_t maskWidth;
    uint32_t maskHeight;
    int32_t maskOffsetX;      // sprite-local X of the mask's left edge (marginLeft on 2024.6+, else 0)
    int32_t maskOffsetY;      // sprite-local Y of the mask's top edge (marginTop on 2024.6+, else 0)
    // Nine-slice (GMS2 sVersion >= 3). Present iff the sprite stored a non-zero nineSliceOffset.
    bool nineSliceEnabled;
    int32_t nsLeft;
    int32_t nsTop;
    int32_t nsRight;
    int32_t nsBottom;
    uint8_t nsTileModes[5];   // order: Left, Top, Right, Bottom, Center. 0=Stretch, 1=Repeat, 2=Mirror, 3=BlankRepeat, 4=Hide
} Sprite;

typedef struct {
    uint32_t count;
    uint32_t parsedCount; // number of sprites loaded from SPRT; slots >= parsedCount are runtime-allocated and own their `name`
    Sprite* sprites;
} SprtChunk;

#endif