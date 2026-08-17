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

#define INFO_FULLSCREEN         (1u << 0)
#define INFO_INTERPOLATE_PIXELS (1u << 1)
#define INFO_USE_NEW_AUDIO      (1u << 2)
#define INFO_NO_BORDER          (1u << 3)
#define INFO_SHOW_CURSOR        (1u << 4)
#define INFO_SIZABLE            (1u << 5)
#define INFO_STAY_ON_TOP        (1u << 6)
#define INFO_CHANGE_RESOLUTION  (1u << 7)
#define INFO_NO_BUTTONS         (1u << 8)
#define INFO_SCREEN_KEY         (1u << 9)
#define INFO_HELP_KEY           (1u << 10)
#define INFO_QUIT_KEY           (1u << 11)
#define INFO_SAVE_KEY           (1u << 12)
#define INFO_SCREENSHOT_KEY     (1u << 13)
#define INFO_CLOSE_SEC          (1u << 14)
#define INFO_FREEZE             (1u << 15)
#define INFO_SHOW_PROGRESS      (1u << 16)
#define INFO_LOAD_TRANSPARENT   (1u << 17)
#define INFO_SCALE_PROGRESS     (1u << 18)
#define INFO_DISPLAY_ERRORS     (1u << 19)
#define INFO_WRITE_ERRORS       (1u << 20)
#define INFO_ABORT_ERRORS       (1u << 21)
#define INFO_VARIABLE_ERRORS    (1u << 22)
#define INFO_CREATION_EVENT_ORDER (1u << 23)

typedef struct {
    int32_t shaderExtensionFlag;
    int32_t shaderExtensionVersion;
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
