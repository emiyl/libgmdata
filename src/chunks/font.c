#include "common.h"

int Font_parse(Reader *reader, DataWin *dw, Font *font, uint32_t fontOptionalCount);
int FontGlyph_parse(Reader *reader, DataWin *dw, FontGlyph *glyph, uint32_t *maxGlyphHeight);
int FontKerningPair_parse(Reader *reader, DataWin *dw, KerningPair *kerningPair);

int FONT_parse(DataWin *dw) {
    Chunk chunk = {0};
    FontChunk *f = &dw->font;

    if (find_chunk(dw, "FONT", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "FONT");

    uint32_t count;
    uint32_t *ptrs;
    Reader_readPointerTable(&reader, &ptrs, &count);
    f->count = count;

    if (count == 0) {
        f->fonts = NULL;
        free(ptrs);
        return 0;
    }

    uint32_t fontOptionalCount = (dw->gen8.wadVersion >= 17) ? 1 : 0;
    {
        size_t baseAfterScaleY = (size_t) ptrs[0] + 40;
        for (uint32_t trial = fontOptionalCount; 4 >= trial; trial++) {
            size_t listStart = baseAfterScaleY + (size_t) trial * 4;
            Reader_seek(&reader, listStart);

            uint32_t probedGlyphCount;
            Reader_readUInt32(&reader, &probedGlyphCount);
            if (probedGlyphCount == 0 || probedGlyphCount > 0x10000) continue;

            uint32_t probedFirstPtr;
            Reader_readUInt32(&reader, &probedFirstPtr);
            size_t expectedFirstPtr = listStart + 4 + (size_t) probedGlyphCount * 4;
            if ((size_t)probedFirstPtr != expectedFirstPtr) continue;
            
            fontOptionalCount = trial;
        }
    }

    f->fonts = (Font*)safeCalloc(count, sizeof(Font));
    repeat(count, i) {
        if (ptrs[i] == 0) continue;
        Reader_seek(&reader, ptrs[i]);
        if (Font_parse(&reader, dw, &f->fonts[i], fontOptionalCount) != 0) {
            free(ptrs);
            free(f->fonts);
            return -1;
        }
    }

    free(ptrs);
    return 0;
}

int Font_parse(Reader *reader, DataWin *dw, Font *f, uint32_t fontOptionalCount) {
    f->present = true;
    Reader_readString(reader, dw, &f->name);
    Reader_readString(reader, dw, &f->displayName);

    uint32_t rawEmSize;
    Reader_readUInt32(reader, &rawEmSize);
    if (rawEmSize & (1u << 31)) {
        float negated;
        memcpy(&negated, &rawEmSize, sizeof(negated));
        f->emSize = -negated;
    } else {
        f->emSize = (float)rawEmSize;
    }
    Reader_readBool32(reader, &f->bold);
    Reader_readBool32(reader, &f->italic);
    Reader_readUInt16(reader, &f->rangeStart);
    Reader_readUInt8(reader, &f->charset);
    Reader_readUInt8(reader, &f->antiAliasing);
    Reader_readUInt32(reader, &f->rangeEnd);
    // Temporarily store the absolute file offset; parseTPAG resolves it in-place to a TPAG index once the TPAG table is known.
    Reader_readInt32(reader, &f->tpagIndex);
    Reader_readFloat32(reader, &f->scaleX);
    Reader_readFloat32(reader, &f->scaleY);
    // Optional fields appear in this order when present: AscenderOffset (WAD17+), Ascender, SDFSpread, LineHeight. `fontOptionalCount` says how many are actually on disk.
    f->ascenderOffset = 0;
    f->ascender = 0;
    f->sdfSpread = 0;
    f->lineHeight = 0;
    f->hasAscender = false;
    f->hasSDFSpread = false;
    f->hasLineHeight = false;
    uint32_t readSoFar = 0;

    if (dw->gen8.wadVersion >= 17 && fontOptionalCount > readSoFar) {
        Reader_readInt32(reader, &f->ascenderOffset);
        readSoFar++;
    }
    if (fontOptionalCount > readSoFar) {
        Reader_readUInt32(reader, &f->ascender);
        f->hasAscender = true;
        readSoFar++;
    }
    if (fontOptionalCount > readSoFar) {
        Reader_readUInt32(reader, &f->sdfSpread);
        f->hasSDFSpread = true;
        readSoFar++;
    }
    if (fontOptionalCount > readSoFar) {
        Reader_readUInt32(reader, &f->lineHeight);
        f->hasLineHeight = true;
        readSoFar++;
    }
    f->isSpriteFont = false;
    f->spriteIndex = -1;
    f->spriteOriginYAdjust = 0;

    uint32_t count;
    uint32_t *ptrs;
    Reader_readPointerTable(reader, &ptrs, &count);
    f->glyphCount = count;
    uint32_t maxGlyphHeight = 0;

    if (count == 0) {
        f->glyphs = NULL;
        f->maxGlyphHeight = maxGlyphHeight;
        free(ptrs);
        return 0;
    }

    f->glyphs = (FontGlyph*)safeCalloc(count, sizeof(FontGlyph));
    repeat(count, i) {
        if (ptrs[i] == 0) continue;
        Reader_seek(reader, ptrs[i]);
        if (FontGlyph_parse(reader, dw, &f->glyphs[i], &maxGlyphHeight) != 0) {
            free(ptrs);
            free(f->glyphs);
            return -1;
        }
    }

    free(ptrs);
    f->maxGlyphHeight = maxGlyphHeight;

    // Build the ASCII fast-path lookup table from font->glyphs.
    // Lets TextUtils_findGlyph skip the linear scan over glyphs[]
    // for the (overwhelmingly common) ASCII case.
    memset(f->glyphLUT, 0, sizeof(f->glyphLUT));
    repeat(f->glyphCount, i) {
        FontGlyph* g = &f->glyphs[i];
        if (128 > g->character && f->glyphLUT[g->character] == NULL) {
            f->glyphLUT[g->character] = g;
        }
    }

    // 512 bytes of trailing padding

    return 0;
}

int FontGlyph_parse(Reader *reader, DataWin *dw, FontGlyph *g, uint32_t *maxGlyphHeight) {
    Reader_readUInt16(reader, &g->character);
    Reader_readUInt16(reader, &g->sourceX);
    Reader_readUInt16(reader, &g->sourceY);
    Reader_readUInt16(reader, &g->sourceWidth);
    Reader_readUInt16(reader, &g->sourceHeight);
    Reader_readInt16(reader, &g->shift);
    Reader_readInt16(reader, &g->offset);
    
    if (g->sourceHeight > *maxGlyphHeight) *maxGlyphHeight = g->sourceHeight;

    // Kerning SimpleShortList (uint16 count)
    Reader_readUInt16(reader, &g->kerningCount);
    if (g->kerningCount > 0) {
        g->kerning = (KerningPair*)safeMalloc(g->kerningCount * sizeof(KerningPair));
        for (uint16_t i = 0; i < g->kerningCount; ++i) {
            if (FontKerningPair_parse(reader, dw, &g->kerning[i]) != 0) {
                free(g->kerning);
                g->kerning = NULL;
                return -1;
            }
        }
    }    else {
        g->kerning = NULL;
    }
    
    return 0;
}

int FontKerningPair_parse(Reader *reader, DataWin *dw, KerningPair *kerningPair) {
    Reader_readInt16(reader, &kerningPair->character);
    Reader_readInt16(reader, &kerningPair->shiftModifier);
    return 0;
}

int FontGlyph_free(FontGlyph *g) {
    if (g == NULL) return 0;
    free(g->kerning);
    g->kerning = NULL;
    return 0;
}

int Font_free(Font *f) {
    if (f == NULL) return 0;
    free((void *)f->name);
    f->name = NULL;
    free((void *)f->displayName);
    f->displayName = NULL;
    repeat(f->glyphCount, i) {
        FontGlyph_free(&f->glyphs[i]);
    }
    free(f->glyphs);
    f->glyphs = NULL;
    return 0;
}

int FONT_free(FontChunk *f) {
    if (f == NULL) return 0;
    repeat(f->count, i) {
        Font_free(&f->fonts[i]);
    }
    free(f->fonts);
    f->fonts = NULL;
    return 0;
}