#include "common.h"
#include "../datawin.h"

static int TPAG_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int TPAG_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);

static int TexturePageItem_parse(Reader *reader, DataWin *dw, TexturePageItem *item) {
    (void)dw;
    item->present = true;
    item->sourceX = 0;
    item->sourceY = 0;
    item->sourceWidth = 0;
    item->sourceHeight = 0;
    item->targetX = 0;
    item->targetY = 0;
    item->targetWidth = 0;
    item->targetHeight = 0;
    item->boundingWidth = 0;
    item->boundingHeight = 0;
    item->texturePageId = -1;

    Reader_readUInt16(reader, &item->sourceX);
    Reader_readUInt16(reader, &item->sourceY);
    Reader_readUInt16(reader, &item->sourceWidth);
    Reader_readUInt16(reader, &item->sourceHeight);
    Reader_readUInt16(reader, &item->targetX);
    Reader_readUInt16(reader, &item->targetY);
    Reader_readUInt16(reader, &item->targetWidth);
    Reader_readUInt16(reader, &item->targetHeight);
    Reader_readUInt16(reader, &item->boundingWidth);
    Reader_readUInt16(reader, &item->boundingHeight);
    Reader_readInt16(reader, &item->texturePageId);
    return 0;
}

int TPAG_parse(DataWin *dw) {
    Chunk chunk = {0};
    TpagChunk *t = &dw->tpag;

    if (get_chunk(dw, "TPAG", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "TPAG");

    return Reader_readAndParsePointerTable(
        &reader, dw,
        (void **)&t->items, NULL,
        &t->count, sizeof(TexturePageItem),
        TPAG_pointerTable_parse,
        TPAG_pointerTable_missingHandler,
        NULL
    );
}

static int TPAG_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)extraData;
    return TexturePageItem_parse(reader, dw, (TexturePageItem *)out);
}

static int TPAG_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; (void)dw; (void)extraData;
    TexturePageItem *item = (TexturePageItem *)out;
    item->present = false;
    item->sourceX = 0;
    item->sourceY = 0;
    item->sourceWidth = 0;
    item->sourceHeight = 0;
    item->targetX = 0;
    item->targetY = 0;
    item->targetWidth = 0;
    item->targetHeight = 0;
    item->boundingWidth = 0;
    item->boundingHeight = 0;
    item->texturePageId = -1;
    return 0;
}

int TPAG_free(TpagChunk *t) {
    if (t == NULL) return -1;
    free(t->items);
    t->items = NULL;
    t->count = 0;
    return 0;
}
