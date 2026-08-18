#ifndef FONT_TYPES_H
#define FONT_TYPES_H

#include <stdint.h>

typedef struct {
    int16_t character;
    int16_t shiftModifier;
} KerningPair;

typedef struct {
    uint16_t character;
    uint16_t sourceX;
    uint16_t sourceY;
    uint16_t sourceWidth;
    uint16_t sourceHeight;
    int16_t shift;
    int16_t offset;
    uint16_t kerningCount;
    KerningPair* kerning;
} FontGlyph;

typedef struct {
    bool present;
    const char* name;
    const char* displayName;
    float emSize;
    bool bold;
    bool italic;
    uint16_t rangeStart;
    uint8_t charset;
    uint8_t antiAliasing;
    uint32_t rangeEnd;
    int32_t tpagIndex;      // resolved TPAG index, -1 if unresolved
    float scaleX;
    float scaleY;
    int32_t ascenderOffset; // wadVersion >= 17 only
    uint32_t ascender;  // GMS 2022.2+ (0 when absent)
    uint32_t sdfSpread; // GMS 2023.2 nonLTS+ (0 when absent)
    uint32_t lineHeight; // GMS 2023.6+ (0 when absent)
    bool hasAscender;
    bool hasSDFSpread;
    bool hasLineHeight;
    uint32_t glyphCount;
    FontGlyph* glyphs;
    uint32_t maxGlyphHeight; // Computed after glyph parse: max sourceHeight across glyphs; HTML5 runner uses this for line stride (see yyFont.TextHeight)
    // ASCII fast-path lookup: glyphLUT[ch] for ch < 128, populated by Font_buildGlyphLUT after glyphs[] is filled.
    // Lets TextUtils_findGlyph skip the linear scan over glyphs[] for the (overwhelmingly common) ASCII case.
    FontGlyph* glyphLUT[128];
    // Sprite font fields (only valid when isSpriteFont is true)
    bool isSpriteFont;
    int32_t spriteIndex; // source sprite index (-1 for regular fonts)
    // Amount to subtract from each glyph's Y at draw time, ONLY used for sprite fonts.
    int16_t spriteOriginYAdjust;
} Font;

typedef struct {
    uint32_t count;
    Font* fonts;
} FontChunk;

#endif