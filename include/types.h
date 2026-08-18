#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "types/gen8.h"
#include "types/optn.h"
#include "types/lang.h"
#include "types/extn.h"
#include "types/sond.h"
#include "types/agrp.h"
#include "types/sprt.h"
#include "types/bgnd.h"
#include "types/path.h"
#include "types/scpt.h"
#include "types/glob.h"
#include "types/shdr.h"
#include "types/font.h"

typedef struct {
    uint8_t *data;
    size_t size;
    size_t offset;
    size_t cursor;
    const char* name; // Optional: Name of the data source for logging/debugging
} Reader;

typedef struct {
    char *text;
    uint32_t offset;
} StringEntry;

typedef struct {
    StringEntry *entries;
    size_t count;
    size_t capacity;
} StringTable;

typedef struct {
    char name[5];
    uint32_t offset;
    uint32_t length;
} Chunk;

typedef struct {
    Chunk *items;
    size_t count;
    size_t capacity;
} ChunkTable;

typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t release;
    uint32_t build;
} DetectedFormat;

typedef struct {
    uint8_t *file_data;
    size_t file_size;
    StringTable strings;
    ChunkTable chunks;

    Gen8Chunk gen8;
    OptnChunk optn;
    LangChunk lang;
    ExtnChunk extn;
    SondChunk sond;
    AgrpChunk agrp;
    SprtChunk sprt;
    BgndChunk bgnd;
    PathChunk path;
    ScptChunk scpt;
    GlobChunk glob;
    ShdrChunk shdr;
    FontChunk font;

    DetectedFormat detected_format;
    uint8_t* mappedFile;

    bool initialized;
} DataWin;

#endif
