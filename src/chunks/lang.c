#include "common.h"

int LANG_parse(DataWin *dw) {
    Chunk chunk = {0};
    LangChunk *l = &dw->lang;

    if (get_chunk(dw, "LANG", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "LANG");

    read(&l->unknown1, UInt32);
    read(&l->languageCount, UInt32);
    read(&l->entryCount, UInt32);

    // Entry IDs
    if (l->entryCount > 0) {
        l->entryIds = (const char **)safeMalloc(l->entryCount * sizeof(const char*));
        repeat(l->entryCount, i) {
            readString(&l->entryIds[i], dw);
        }
    } else {
        l->entryIds = NULL;
    }

    // Languages
    if (l->languageCount > 0) {
        l->languages = (Language *)safeMalloc(l->languageCount * sizeof(Language));
        repeat(l->languageCount, i) {
            readString(&l->languages[i].name, dw);
            readString(&l->languages[i].region, dw);
            l->languages[i].entryCount = l->entryCount;
            if (l->entryCount > 0) {
                l->languages[i].entries = (const char **)safeMalloc(l->entryCount * sizeof(const char*));
                repeat(l->entryCount, j) {
                    readString(&l->languages[i].entries[j], dw);
                }
            } else {
                l->languages[i].entries = NULL;
            }
        }
    } else {
        l->languages = NULL;
    }

    return 0;
}

int LANG_free(LangChunk *l) {
    if (l == NULL) {
        return -1;
    }

    if (l->entryIds != NULL) {
        free(l->entryIds);
        l->entryIds = NULL;
    }

    if (l->languages != NULL) {
        for (uint32_t i = 0; i < l->languageCount; ++i) {
            if (l->languages[i].entries != NULL) {
                free(l->languages[i].entries);
                l->languages[i].entries = NULL;
            }
        }
        free(l->languages);
        l->languages = NULL;
    }
    return 0;
}