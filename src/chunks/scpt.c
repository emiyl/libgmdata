#include "common.h"

int Script_parse(Reader *reader, DataWin *dw, Script *script);

int SCPT_parse(DataWin *dw) {
    Chunk chunk = {0};
    Scpt *s = &dw->scpt;

    if (find_chunk(dw, "SCPT", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "SCPT");

    uint32_t count;
    uint32_t *ptrs;
    Reader_readPointerTable(&reader, &ptrs, &count);
    s->count = count;

    if (count == 0) {
        s->scripts = NULL;
        free(ptrs);
        return 0;
    }

    s->scripts = (Script*)safeCalloc(count, sizeof(Script));
    repeat(count, i) {
        if (ptrs[i] == 0) continue;
        Reader_seek(&reader, ptrs[i]);
        if (Script_parse(&reader, dw, &s->scripts[i]) != 0) {
            free(ptrs);
            free(s->scripts);
            return -1;
        }
    }

    free(ptrs);
    return 0;
}

int Script_parse(Reader *reader, DataWin *dw, Script *script) {
    script->present = true;
    Reader_readString(reader, dw, &script->name);
    Reader_readInt32(reader, &script->codeId);
    return 0;
}

int Script_free(Script *script) {
    script->name = NULL;
    return 0;
}

int SCPT_free(Scpt *scpt) {
    repeat(scpt->count, i) {
        Script_free(&scpt->scripts[i]);
    }
    free(scpt->scripts);
    scpt->scripts = NULL;
    scpt->count = 0;
    return 0;
}