#ifndef GLOB_TYPES_H
#define GLOB_TYPES_H

#include <stdint.h>

typedef struct {
    uint32_t count;
    int32_t* codeIds;
} GlobChunk;

#endif