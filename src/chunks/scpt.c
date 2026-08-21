#include "common.h"

static int SCPT_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int SCPT_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
int SCPT_parse(DataWin *dw) {
    Chunk chunk = {0};
    ScptChunk *s = &dw->scpt;

    if (find_chunk(dw, "SCPT", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "SCPT");

    uint32_t *ptrs = NULL;
    if (Reader_readPointerTable(&reader, &ptrs, &s->count) != 0)
        return -1;

    if (s->count == 0) {
        s->scripts = NULL;
        free(ptrs);
        return 0;
    }
    
    int result = Reader_pointerTable_parse(
        &reader, dw,
        ptrs, s->count,
        (void **)&s->scripts, sizeof(Script),
        NULL,
        SCPT_pointerTable_parse,
        SCPT_pointerTable_missingHandler,
        NULL
    );

    free(ptrs);
    return result;
}

static int Script_parse(Reader *reader, DataWin *dw, Script *script) {
    script->present = true;
    Reader_readString(reader, dw, &script->name);
    Reader_readInt32(reader, &script->codeId);
    return 0;
}

static int SCPT_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)extraData; // Unused parameter
    return Script_parse(reader, dw, (Script *)out);
}

static int SCPT_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[SCPT_pointerTable_missingHandler] Script pointer is missing, initializing default values.\n");

    Script *script = (Script *)out;
    script->present = false;
    script->name = NULL;
    script->codeId = 0;

    return 0;
}

static int Script_free(Script *script) {
    script->name = NULL;
    return 0;
}

int SCPT_free(ScptChunk *scpt) {
    repeat(scpt->count, i) {
        Script_free(&scpt->scripts[i]);
    }
    free(scpt->scripts);
    scpt->scripts = NULL;
    scpt->count = 0;
    return 0;
}