#ifndef SOND_TYPES_H
#define SOND_TYPES_H

#include <stdint.h>

typedef enum {
    AUDIO_ENTRY_FLAG_IS_EMBEDDED = 0x01,
    AUDIO_ENTRY_FLAG_IS_COMPRESSED = 0x02,
    AUDIO_ENTRY_FLAG_IS_DECOMPRESSED_ON_LOAD = 0x03,
    AUDIO_ENTRY_FLAG_REGULAR = 0x64
} AudioEntryFlags;

typedef struct {
    bool present;
    const char* name;
    uint32_t flags;
    const char* type;
    const char* file;
    uint32_t effects;
    float volume;
    float pitch;
    float pan; // -1.0 = full left, 0.0 = center, +1.0 = full right. Legacy field that is not used in WAD11+.
    int32_t audioGroup;
    int32_t audioFile;
} Sound;

typedef struct {
    uint32_t count;
    Sound* sounds;
} Sond;

#endif