#include "common.h"
#include "../datawin.h"

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

    // GM 2024.14+ added a "path" parameter for each AudioGroup
    // To detect it, we'll check if the difference between two pointers is 8 (two int32)
    // We CAN'T figure out if there aren't at least two AudioGroups, but for any meaningful purposes any game that has external AudioGroups WILL have
    // at least two entries, one for the default AudioGroup and another for the external AudioGroup
    if (DataWin_isVersionAtLeast(dw, 2024, 13, 0, 0)) {
        if (a->count >= 2) {
            uint32_t diff = ptrs[1] - ptrs[0];

            if (diff >= 8) {
                DataWin_bumpVersionTo(dw, 2024, 14, 0, 0);
            }
        } else if (a->count == 1) {
            // If there's only one entry, we CAN'T figure out easily based on the pointer diffs
            // But here's the trick: We can read it twice, if the path is null for the FIRST audiogroup, then it is NOT 2024.14
            if (ptrs[0] == 0) {
                // Somehow in a empty GameMaker 2026.0.0.23 game the pointer can be 0 even though it has one audio group...?
                // If that's the case, we'll just bail out
                free(ptrs);
                a->audioGroups = NULL;
                a->count = 0;
                return 0;
            }

            Reader_seek(&reader, ptrs[0]);
            const char* name;
            const char* path;
            Reader_readString(&reader, dw, &name);
            Reader_readString(&reader, dw, &path);

            if (strcmp(name, "audiogroup_default") == 0 && path != NULL) {
                DataWin_bumpVersionTo(dw, 2024, 14, 0, 0);
            }
        }
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
    if (Reader_readString(reader, dw, &ag->name) != 0) return -1;
    if (DataWin_isVersionAtLeast(dw, 2024, 14, 0, 0)) {
        if (Reader_readString(reader, dw, &ag->path) != 0) return -1;
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