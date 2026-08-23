#include "common.h"

int SEQN_parse(DataWin *dw) {
    Chunk chunk = {0};
    SeqnChunk *s = &dw->seqn;

    if (get_chunk(dw, "SEQN", &chunk) != 0) {
        s->count = 0;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    s->count = chunk.length == 0U ? 0U : 1U;
    return 0;
}

int SEQN_free(SeqnChunk *s) {
    if (s == NULL) {
        return -1;
    }
    s->count = 0;
    return 0;
}
