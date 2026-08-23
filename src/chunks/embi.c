#include "common.h"

int EMBI_parse(DataWin *dw) {
    Chunk chunk = {0};
    EmbiChunk *e = &dw->embi;

    if (get_chunk(dw, "EMBI", &chunk) != 0) {
        e->count = 0;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    e->count = chunk.length == 0U ? 0U : 1U;
    return 0;
}

int EMBI_free(EmbiChunk *e) {
    if (e == NULL) {
        return -1;
    }
    e->count = 0;
    return 0;
}
