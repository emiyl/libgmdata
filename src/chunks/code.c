#include "common.h"

static int CODE_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData);
static int CODE_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData);
int CODE_free(CodeChunk *c);

int CODE_parse(DataWin *dw) {
    Chunk chunk = {0};
    CodeChunk *c = &dw->code;

    if (get_chunk(dw, "CODE", &chunk) != 0) {
        c->count = 0;
        c->entries = NULL;
        c->bytecodeData = NULL;
        c->bytecodeBase = 0;
        c->bytecodeSize = 0;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "CODE");

    uint32_t *codePtrs = NULL;
    uint32_t codeCount = 0;
    if (Reader_readPointerTable(&reader, &codePtrs, &codeCount) != 0) {
        return -1;
    }

    c->count = codeCount;
    if (codeCount == 0) {
        free(codePtrs);
        c->entries = NULL;
        c->bytecodeData = NULL;
        c->bytecodeBase = 0;
        c->bytecodeSize = 0;
        return 0;
    }

    bool oldFormat = dw->gen8.wadVersion <= 14;
    if (Reader_parsePointerTableParallel(
        &reader, dw,
        codePtrs, codeCount,
        (void **)&c->entries, sizeof(CodeEntry),
        NULL,
        CODE_pointerTable_parse,
        CODE_pointerTable_missingHandler,
        NULL
    ) != 0) {
        free(codePtrs);
        CODE_free(c);
        return -1;
    }

    free(codePtrs);

    size_t chunkEnd = chunk.offset + chunk.length;
    if (oldFormat) {
        c->bytecodeBase = (uint32_t)chunk.offset;
        c->bytecodeSize = chunk.length;
        c->bytecodeData = (uint8_t *)safeMalloc(chunk.length);
        memcpy(c->bytecodeData, dw->file_data + chunk.offset, chunk.length);
        return 0;
    }

    uint32_t blobStart = UINT32_MAX;
    repeat(c->count, i) {
        if (!c->entries[i].present) {
            continue;
        }
        if (blobStart > c->entries[i].bytecodeAbsoluteOffset) {
            blobStart = c->entries[i].bytecodeAbsoluteOffset;
        }
    }

    if (blobStart == UINT32_MAX) {
        blobStart = (uint32_t)chunk.offset;
    }

    size_t blobSize = chunkEnd - blobStart;
    c->bytecodeBase = blobStart;
    c->bytecodeSize = blobSize;
    c->bytecodeData = (uint8_t *)safeMalloc(blobSize);
    memcpy(c->bytecodeData, dw->file_data + blobStart, blobSize);

    return 0;
}

static int CODE_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)extraData;
    if (reader == NULL || dw == NULL || out == NULL) {
        return -1;
    }

    CodeEntry *entry = (CodeEntry *)out;
    memset(entry, 0, sizeof(*entry));

    entry->present = true;
    readString(&entry->name, dw);
    read(&entry->length, UInt32);

    if (dw->gen8.wadVersion <= 14) {
        entry->localsCount = 0;
        entry->argumentsCount = 0;
        entry->offset = 0;
        entry->bytecodeAbsoluteOffset = (uint32_t)(reader->offset + reader->cursor);
        if (Reader_skip(reader, (int)entry->length) != 0) {
            return -1;
        }
        return 0;
    }

    uint16_t localsCount = 0;
    uint16_t argumentsCount = 0;
    read(&localsCount, UInt16);
    read(&argumentsCount, UInt16);

    entry->localsCount = localsCount;
    entry->argumentsCount = argumentsCount;

    size_t relAddrFieldPos = reader->offset + reader->cursor;
    int32_t bytecodeRelAddr = 0;
    read(&bytecodeRelAddr, Int32);
    entry->bytecodeAbsoluteOffset = (uint32_t)((int64_t)relAddrFieldPos + bytecodeRelAddr);

    read(&entry->offset, UInt32);

    return 0;
}

static int CODE_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)reader; (void)dw; (void)extraData;

    CodeEntry *entry = (CodeEntry *)out;
    memset(entry, 0, sizeof(*entry));
    entry->present = false;
    entry->name = NULL;
    entry->length = 0;
    entry->localsCount = 0;
    entry->argumentsCount = 0;
    entry->offset = 0;
    entry->bytecodeAbsoluteOffset = 0;
    return 0;
}

int CODE_free(CodeChunk *c) {
    if (c == NULL) {
        return -1;
    }

    if (c->entries != NULL) {
        repeat(c->count, i) {
            if (c->entries[i].name != NULL) {
                free((void *)c->entries[i].name);
                c->entries[i].name = NULL;
            }
        }
        free(c->entries);
        c->entries = NULL;
    }

    free(c->bytecodeData);
    c->bytecodeData = NULL;
    c->bytecodeBase = 0;
    c->bytecodeSize = 0;
    c->count = 0;
    return 0;
}
