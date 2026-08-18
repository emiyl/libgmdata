#include "common.h"
#include "../datawin.h"

int AudioGroup_parse(Reader *reader, DataWin *dw, AudioGroup *ag);

int AGRP_parse(DataWin *dw) {
    Chunk chunk = {0};
    Agrp *a = &dw->agrp;

    if (find_chunk(dw, "AGRP", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "AGRP");

    uint32_t count;
    uint32_t* ptrs;
    if (Reader_readPointerTable(&reader, &ptrs, &count) != 0) return -1;
    a->count = count;

    if (count == 0) {
        a->audioGroups = NULL;
        free(ptrs);
        return 0; // Success
    }

    // GM 2024.14+ added a "path" parameter for each AudioGroup
    // To detect it, we'll check if the difference between two pointers is 8 (two int32)
    // We CAN'T figure out if there aren't at least two AudioGroups, but for any meaningful purposes any game that has external AudioGroups WILL have
    // at least two entries, one for the default AudioGroup and another for the external AudioGroup
    if (DataWin_isVersionAtLeast(dw, 2024, 13, 0, 0)) {
        if (count >= 2) {
            uint32_t diff = ptrs[1] - ptrs[0];

            if (diff >= 8) {
                DataWin_bumpVersionTo(dw, 2024, 14, 0, 0);
            }
        } else if (count == 1) {
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

    a->audioGroups = (AudioGroup *)safeCalloc(count, sizeof(AudioGroup));

    repeat(count, i) {
        if (ptrs[i] == 0) continue;
        Reader_seek(&reader, ptrs[i]);
        if (AudioGroup_parse(&reader, dw, &a->audioGroups[i]) != 0) {
            free(ptrs);
            return -1;
        }
    }
    free(ptrs);

    return 0;
}

int AudioGroup_parse(Reader *reader, DataWin *dw, AudioGroup *ag) {
    ag->present = true;
    if (Reader_readString(reader, dw, &ag->name) != 0) return -1;
    if (DataWin_isVersionAtLeast(dw, 2024, 14, 0, 0)) {
        if (Reader_readString(reader, dw, &ag->path) != 0) return -1;
    }
    return 0;
}

void AudioGroup_free(AudioGroup *ag) {
    if (ag->name) free((void*)ag->name);
    if (ag->path) free((void*)ag->path);
}

void AGRP_free(Agrp *a) {
    if (a->audioGroups) {
        repeat(a->count, i) {
            AudioGroup_free(&a->audioGroups[i]);
        }
        free(a->audioGroups);
        a->audioGroups = NULL;
    }
    a->count = 0;
}