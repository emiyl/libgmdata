#ifndef EXTN_TYPES_H
#define EXTN_TYPES_H

#include <stdint.h>

typedef struct {
    const char* name;
    uint32_t id;
    uint32_t kind;
    uint32_t retType;
    const char* extName;
    uint32_t argumentCount;
    uint32_t* arguments;
} ExtensionFunction;

typedef struct {
    const char* filename;
    const char* cleanupScript;
    const char* initScript;
    uint32_t kind;
    uint32_t functionCount;
    ExtensionFunction* functions;
} ExtensionFile;

typedef struct {
    const char* folderName;
    const char* name;
    const char* className;
    uint32_t fileCount;
    ExtensionFile* files;
} Extension;

typedef struct {
    uint32_t count;
    Extension* extensions;
} ExtnChunk;

#endif