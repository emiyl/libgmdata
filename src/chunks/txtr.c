#include "common.h"

static int TXTR_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)extraData; (void)dw;
    Texture *tex = (Texture *)out;
    memset(tex, 0, sizeof(*tex));

    tex->present = true;
    read(&tex->scaled, UInt32);
    read(&tex->generatedMips, UInt32);
    read(&tex->textureBlockSize, UInt32);
    read(&tex->textureWidth, Int32);
    read(&tex->textureHeight, Int32);
    read(&tex->indexInGroup, Int32);
    
    return 0;
}

static int TXTR_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)reader; (void)dw; (void)extraData;
    Texture *tex = (Texture *)out;
    memset(tex, 0, sizeof(*tex));
    tex->present = false;
    return 0;
}

int TXTR_parse(DataWin *dw) {
    Chunk chunk = {0};
    TxtrChunk *t = &dw->txtr;

    if (get_chunk(dw, "TXTR", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "TXTR");

    return Reader_readAndParsePointerTable(
        &reader, dw,
        (void **)&t->textures, NULL,
        &t->count, sizeof(Texture),
        TXTR_pointerTable_parse,
        TXTR_pointerTable_missingHandler,
        NULL
    );
}

int TXTR_free(TxtrChunk *t) {
    if (t == NULL) return -1;
    free(t->textures);
    t->textures = NULL;
    t->count = 0;
    return 0;
}
