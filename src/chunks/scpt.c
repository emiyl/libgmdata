#include "common.h"

static int SCPT_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int SCPT_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
int SCPT_parse(DataWin *dw) {
    Chunk chunk = {0};
    ScptChunk *s = &dw->scpt;

    if (get_chunk(dw, "SCPT", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "SCPT");

    return Reader_readAndParsePointerTable(
        reader, dw,
        (void **)&s->scripts, NULL,
        &s->count, sizeof(Script),
        SCPT_pointerTable_parse,
        SCPT_pointerTable_missingHandler,
        NULL
    );
}

static int Script_parse(Reader *reader, DataWin *dw, Script *script) {
    script->present = true;
    readString(&script->name, dw);
    read(&script->codeId, Int32);
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