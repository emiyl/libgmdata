#ifndef OPTN_TYPES_H
#define OPTN_TYPES_H

#include <stdint.h>

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

#endif