#include "common.h"

static int AUDO_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)extraData;
    AudioEntry *entry = (AudioEntry *)out;
    memset(entry, 0, sizeof(*entry));
    entry->present = true;
    if (Reader_readUInt32(reader, &entry->dataSize) != 0) return -1;
    entry->dataOffset = (uint32_t)reader->cursor;
    entry->data = NULL;
    return 0;
}

static int AUDO_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)reader; (void)dw; (void)extraData;
    AudioEntry *entry = (AudioEntry *)out;
    memset(entry, 0, sizeof(*entry));
    entry->present = false;
    return 0;
}

int AUDO_parse(DataWin *dw) {
    Chunk chunk = {0};
    AudoChunk *a = &dw->audo;

    if (get_chunk(dw, "AUDO", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "AUDO");

    return Reader_readAndParsePointerTable(
        &reader, dw,
        (void **)&a->entries, NULL,
        &a->count, sizeof(AudioEntry),
        AUDO_pointerTable_parse,
        AUDO_pointerTable_missingHandler,
        NULL
    );
}

int AUDO_free(AudoChunk *a) {
    if (a == NULL) return -1;
    free(a->entries);
    a->entries = NULL;
    a->count = 0;
    return 0;
}
