#include "common.h"

static int SHDR_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int SHDR_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
int SHDR_parse(DataWin *dw) {
    Chunk chunk = {0};
    ShdrChunk *s = &dw->shdr;

    if (get_chunk(dw, "SHDR", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "SHDR");

    return Reader_readAndParsePointerTable(
        reader, dw,
        (void **)&s->shaders, NULL,
        
        &s->count, sizeof(Shader),
        SHDR_pointerTable_parse,
        SHDR_pointerTable_missingHandler,
        NULL
    );
}

int Shader_parse(Reader *reader, DataWin *dw, Shader *sh) {
    if (sh == NULL) {
        logError("[Shader_parse] sh is NULL\n");
        return -1;
    }

    sh->present = true;
    readString(&sh->name, dw);
    read(&sh->type, UInt32);
    readString(&sh->glslES_Vertex, dw);
    readString(&sh->glslES_Fragment, dw);
    readString(&sh->glsl_Vertex, dw);
    readString(&sh->glsl_Fragment, dw);
    readString(&sh->hlsl9_Vertex, dw);
    readString(&sh->hlsl9_Fragment, dw);
    read(&sh->hlsl11_VertexOffset, UInt32);
    read(&sh->hlsl11_PixelOffset, UInt32);

    // Vertex attributes SimpleList
    read(&sh->vertexAttributeCount, UInt32);
    sh->vertexAttributes = (const char **)safeCalloc(sh->vertexAttributeCount, sizeof(const char *));
    repeat(sh->vertexAttributeCount, i) {
        readString(&sh->vertexAttributes[i], dw);
    }

    // Version field and console shader variants only exist on wadVersion > 13.
    if (dw->gen8.wadVersion > 13) {
        read(&sh->version, Int32);
        read(&sh->pssl_VertexOffset, UInt32);
        read(&sh->pssl_VertexLen, UInt32);
        read(&sh->pssl_PixelOffset, UInt32);
        read(&sh->pssl_PixelLen, UInt32);
        read(&sh->cgVita_VertexOffset, UInt32);
        read(&sh->cgVita_VertexLen, UInt32);
        read(&sh->cgVita_PixelOffset, UInt32);
        read(&sh->cgVita_PixelLen, UInt32);
        
        if (sh->version >= 2) {
            read(&sh->cgPS3_VertexOffset, UInt32);
            read(&sh->cgPS3_VertexLen, UInt32);
            read(&sh->cgPS3_PixelOffset, UInt32);
            read(&sh->cgPS3_PixelLen, UInt32);
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

static int SHDR_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)extraData; // Unused parameter
    return Shader_parse(reader, dw, (Shader *)out);
}

static int SHDR_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[SHDR_pointerTable_missingHandler] Shader pointer is missing, initializing default values.\n");

    Shader *sh = (Shader *)out;
    sh->present = false;
    sh->name = NULL;
    sh->type = 0;
    sh->glslES_Vertex = NULL;
    sh->glslES_Fragment = NULL;
    sh->glsl_Vertex = NULL;
    sh->glsl_Fragment = NULL;
    sh->hlsl9_Vertex = NULL;
    sh->hlsl9_Fragment = NULL;
    sh->hlsl11_VertexOffset = 0;
    sh->hlsl11_PixelOffset = 0;
    sh->vertexAttributeCount = 0;
    sh->vertexAttributes = NULL;

    return 0;
}

static int Shader_free(Shader *sh) {
    if (sh == NULL) return -1;

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
    return 0;
}

int SHDR_free(ShdrChunk *s) {
    if (s == NULL) return -1;

    int result = 0;
    repeat(s->count, i) {
        if (Shader_free(&s->shaders[i])) {
            logWarn("[SHDR_free] Failed to free Shader at index %u\n", i);
            result = -1;
        }
    }
    free(s->shaders);
    s->shaders = NULL;
    s->count = 0;
    return result;
}