#ifndef SCPT_TYPES_H
#define SCPT_TYPES_H

#include <stdint.h>

typedef struct {
    bool present;
    const char* name;
    int32_t codeId;
} Script;

typedef struct {
    uint32_t count;
    Script* scripts;
} ScptChunk;

#endif