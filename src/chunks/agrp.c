#include "common.h"

static int AGRP_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int AGRP_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
int AGRP_parse(DataWin *dw) {
    Chunk chunk = {0};
    AgrpChunk *a = &dw->agrp;

    if (get_chunk(dw, "AGRP", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "AGRP");

    uint32_t* ptrs = NULL;
    if (Reader_readPointerTable(&reader, &ptrs, &a->count) != 0)
        return -1;
    
    if (a->count == 0) {
        a->audioGroups = NULL;
        free(ptrs);
        return 0; // Success
    }
    
    int result = Reader_parsePointerTable(
        &reader, dw,
        ptrs, a->count,
        (void **)&a->audioGroups, sizeof(AudioGroup),
        NULL,
        AGRP_pointerTable_parse,
        AGRP_pointerTable_missingHandler,
        NULL
    );

    free(ptrs);
    return result;
}

static int AudioGroup_parse(Reader *reader, DataWin *dw, AudioGroup *ag) {
    ag->present = true;
    readString(&ag->name, dw);
    if (DataWin_isVersionAtLeast(dw, 2024, 14, 0, 0)) {
        readString(&ag->path, dw);
    }
    return 0;
}

static int AGRP_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)extraData; // Unused parameter
    return AudioGroup_parse(reader, dw, out);
}

static int AGRP_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; // Unused parameter
    (void)dw; // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[AGRP_pointerTable_missingHandler] Audio group pointer is missing, initializing default values.\n");

    AudioGroup *ag = (AudioGroup *)out;
    ag->present = false;
    ag->name = NULL;
    ag->path = NULL;
    return 0;
}

static int AudioGroup_free(AudioGroup *ag) {
    if (ag->name) free((void*)ag->name);
    if (ag->path) free((void*)ag->path);
    return 0;
}

int AGRP_free(AgrpChunk *a) {
    int result = 0;
    if (a->audioGroups) {
        repeat(a->count, i) {
            if(AudioGroup_free(&a->audioGroups[i])) {
                logWarn("[AGRP_free] Failed to free AudioGroup at index %u\n", i);
                result = -1;
            };
        }
        free(a->audioGroups);
        a->audioGroups = NULL;
    }
    a->count = 0;
    return result;
}