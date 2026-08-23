#include "common.h"

int PSYS_parse(DataWin *dw) {
    Chunk chunk = {0};
    PsysChunk *p = &dw->psys;

    if (get_chunk(dw, "PSYS", &chunk) != 0) {
        p->count = 0;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    p->count = chunk.length == 0U ? 0U : 1U;
    return 0;
}

int PSYS_free(PsysChunk *p) {
    if (p == NULL) {
        return -1;
    }
    p->count = 0;
    return 0;
}
