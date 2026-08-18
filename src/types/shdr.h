#ifndef SHDR_TYPES_H
#define SHDR_TYPES_H

#include <stdint.h>

typedef struct {
    bool present;
    const char* name;
    uint32_t type;
    const char* glslES_Vertex;
    const char* glslES_Fragment;
    const char* glsl_Vertex;
    const char* glsl_Fragment;
    const char* hlsl9_Vertex;
    const char* hlsl9_Fragment;
    uint32_t hlsl11_VertexOffset;
    uint32_t hlsl11_PixelOffset;
    uint32_t vertexAttributeCount;
    const char** vertexAttributes;
    int32_t version;
    uint32_t pssl_VertexOffset;
    uint32_t pssl_VertexLen;
    uint32_t pssl_PixelOffset;
    uint32_t pssl_PixelLen;
    uint32_t cgVita_VertexOffset;
    uint32_t cgVita_VertexLen;
    uint32_t cgVita_PixelOffset;
    uint32_t cgVita_PixelLen;
    uint32_t cgPS3_VertexOffset;
    uint32_t cgPS3_VertexLen;
    uint32_t cgPS3_PixelOffset;
    uint32_t cgPS3_PixelLen;
} Shader;

typedef struct {
    uint32_t count;
    Shader* shaders;
} Shdr;

#endif