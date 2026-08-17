#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *data;
    size_t size;
    size_t cursor;
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
    uint8_t isDebuggerDisabled;
    uint8_t wadVersion;
    const char* fileName;
    const char* config;
    uint32_t lastObj;
    uint32_t lastTile;
    uint32_t gameID;
    uint8_t directPlayGuid[16];
    const char* name;
    uint32_t major;
    uint32_t minor;
    uint32_t release;
    uint32_t build;
    uint32_t defaultWindowWidth;
    uint32_t defaultWindowHeight;
    uint32_t info;
    uint32_t licenseCRC32;
    uint8_t licenseMD5[16];
    uint64_t timestamp;
    const char* displayName;
    uint64_t activeTargets;
    uint64_t functionClassifications;
    int32_t steamAppID;
    uint32_t debuggerPort;
    uint32_t roomOrderCount;
    int32_t* roomOrder;
    float gms2FPS;
} Gen8;

typedef struct {
    const char* name;
    const char* value;
} OptnConstant;

typedef struct {
    uint64_t info;
    int32_t scale;
    uint32_t windowColor;
    uint32_t colorDepth;
    uint32_t resolution;
    uint32_t frequency;
    uint32_t vertexSync;
    uint32_t priority;
    uint32_t backImage;
    uint32_t frontImage;
    uint32_t loadImage;
    uint32_t loadAlpha;
    uint32_t constantCount;
    OptnConstant* constants;
} Optn;

typedef struct {
    uint8_t *file_data;
    size_t file_size;
    StringTable strings;
    ChunkTable chunks;

    Gen8 gen8;
    Optn optn;

    bool initialized;
} DataWin;

#endif
