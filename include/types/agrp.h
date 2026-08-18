#ifndef AGRP_TYPES_H
#define AGRP_TYPES_H

#include <stdint.h>

typedef struct {
    bool present;
    const char* name;
    const char* path; // nullptr for pre-GM 2024.14+ games
} AudioGroup;

typedef struct {
    uint32_t count;
    AudioGroup* audioGroups;
} AgrpChunk;

#endif