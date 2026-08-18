#include "common.h"

int Shader_parse(Reader *reader, DataWin *dw, Shader *shader);

int SHDR_parse(DataWin *dw) {
    Chunk chunk = {0};
    Shdr *s = &dw->shdr;

    if (find_chunk(dw, "SHDR", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "SHDR");

    uint32_t count;
    uint32_t *ptrs;
    if (Reader_readPointerTable(&reader, &ptrs, &count) != 0) return -1;
    s->count = count;

    s->shaders = (Shader *)safeCalloc(count, sizeof(Shader));
    repeat(count, i) {
        if (ptrs[i] == 0) {
            s->shaders[i].present = false;
            continue;
        }
        Reader_seek(&reader, ptrs[i]);
        if (Shader_parse(&reader, dw, &s->shaders[i]) != 0) {
            free(ptrs);
            logError("[SHDR_parse] Failed to parse shader at index %u\n", i);
            return -1;
        }
    }

    return 0;
}

int Shader_parse(Reader *reader, DataWin *dw, Shader *sh) {
    if (sh == NULL) {
        logError("[Shader_parse] sh is NULL\n");
        return -1;
    }

    sh->present = true;
    Reader_readString(reader, dw, &sh->name);
    Reader_readUInt32(reader, &sh->type);
    Reader_readString(reader, dw, &sh->glslES_Vertex);
    Reader_readString(reader, dw, &sh->glslES_Fragment);
    Reader_readString(reader, dw, &sh->glsl_Vertex);
    Reader_readString(reader, dw, &sh->glsl_Fragment);
    Reader_readString(reader, dw, &sh->hlsl9_Vertex);
    Reader_readString(reader, dw, &sh->hlsl9_Fragment);
    Reader_readUInt32(reader, &sh->hlsl11_VertexOffset);
    Reader_readUInt32(reader, &sh->hlsl11_PixelOffset);

    // Vertex attributes SimpleList
    Reader_readUInt32(reader, &sh->vertexAttributeCount);
    if (sh->vertexAttributeCount > 0) {
        sh->vertexAttributes = (const char **)safeCalloc(sh->vertexAttributeCount, sizeof(const char *));
        repeat(sh->vertexAttributeCount, i) {
            Reader_readString(reader, dw, &sh->vertexAttributes[i]);
        }
    } else {
        sh->vertexAttributes = NULL;
    }

    // Version field and console shader variants only exist on wadVersion > 13.
    if (dw->gen8.wadVersion > 13) {

        Reader_readInt32(reader, &sh->version);
        Reader_readUInt32(reader, &sh->pssl_VertexOffset);
        Reader_readUInt32(reader, &sh->pssl_VertexLen);
        Reader_readUInt32(reader, &sh->pssl_PixelOffset);
        Reader_readUInt32(reader, &sh->pssl_PixelLen);
        Reader_readUInt32(reader, &sh->cgVita_VertexOffset);
        Reader_readUInt32(reader, &sh->cgVita_VertexLen);
        Reader_readUInt32(reader, &sh->cgVita_PixelOffset);
        Reader_readUInt32(reader, &sh->cgVita_PixelLen);
        
        if (sh->version >= 2) {
            Reader_readUInt32(reader, &sh->cgPS3_VertexOffset);
            Reader_readUInt32(reader, &sh->cgPS3_VertexLen);
            Reader_readUInt32(reader, &sh->cgPS3_PixelOffset);
            Reader_readUInt32(reader, &sh->cgPS3_PixelLen);
        } else {
            sh->cgPS3_VertexOffset = 0;
            sh->cgPS3_VertexLen = 0;
            sh->cgPS3_PixelOffset = 0;
            sh->cgPS3_PixelLen = 0;
        }
    } else {
        sh->version = 0;
        sh->pssl_VertexOffset = 0;
        sh->pssl_VertexLen = 0;
        sh->pssl_PixelOffset = 0;
        sh->pssl_PixelLen = 0;
        sh->cgVita_VertexOffset = 0;
        sh->cgVita_VertexLen = 0;
        sh->cgVita_PixelOffset = 0;
        sh->cgVita_PixelLen = 0;
        sh->cgPS3_VertexOffset = 0;
        sh->cgPS3_VertexLen = 0;
        sh->cgPS3_PixelOffset = 0;
        sh->cgPS3_PixelLen = 0;
    }

    // Blob data follows but we skip it (pointer list seeking handles position)
    return 0;
}

void Shader_free(Shader *sh) {
    if (sh == NULL) return;

    free((void *)sh->name);
    sh->name = NULL;

    free((void *)sh->glslES_Vertex);
    sh->glslES_Vertex = NULL;

    free((void *)sh->glslES_Fragment);
    sh->glslES_Fragment = NULL;

    free((void *)sh->glsl_Vertex);
    sh->glsl_Vertex = NULL;

    free((void *)sh->glsl_Fragment);
    sh->glsl_Fragment = NULL;

    free((void *)sh->hlsl9_Vertex);
    sh->hlsl9_Vertex = NULL;

    free((void *)sh->hlsl9_Fragment);
    sh->hlsl9_Fragment = NULL;

    if (sh->vertexAttributes != NULL) {
        repeat(sh->vertexAttributeCount, i) {
            free((void *)sh->vertexAttributes[i]);
            sh->vertexAttributes[i] = NULL;
        }
        free(sh->vertexAttributes);
        sh->vertexAttributes = NULL;
        sh->vertexAttributeCount = 0;
    }
}

void SHDR_free(Shdr *s) {
    if (s == NULL) return;

    repeat(s->count, i) {
        Shader_free(&s->shaders[i]);
    }
    free(s->shaders);
    s->shaders = NULL;
    s->count = 0;
}