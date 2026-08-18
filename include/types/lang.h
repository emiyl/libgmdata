#ifndef LANG_TYPES_H
#define LANG_TYPES_H

#include <stdint.h>

typedef struct {
    const char* name;
    const char* region;
    uint32_t entryCount;
    const char** entries;
} Language;

typedef struct {
    uint32_t unknown1;
    uint32_t languageCount;
    uint32_t entryCount;
    const char** entryIds;
    Language* languages;
} LangChunk;

#endif