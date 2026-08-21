#include "common.h"

int LANG_parse(DataWin *dw) {
    Chunk chunk = {0};
    LangChunk *l = &dw->lang;

    if (get_chunk(dw, "LANG", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "LANG");

    Reader_readUInt32(&reader, &l->unknown1);
    Reader_readUInt32(&reader, &l->languageCount);
    Reader_readUInt32(&reader, &l->entryCount);

    // Entry IDs
    if (l->entryCount > 0) {
        l->entryIds = (const char **)safeMalloc(l->entryCount * sizeof(const char*));
        repeat(l->entryCount, i) {
            Reader_readString(&reader, dw, &l->entryIds[i]);
        }
    } else {
        l->entryIds = NULL;
    }

    // Languages
    if (l->languageCount > 0) {
        l->languages = (Language *)safeMalloc(l->languageCount * sizeof(Language));
        repeat(l->languageCount, i) {
            Reader_readString(&reader, dw, &l->languages[i].name);
            Reader_readString(&reader, dw, &l->languages[i].region);
            l->languages[i].entryCount = l->entryCount;
            if (l->entryCount > 0) {
                l->languages[i].entries = (const char **)safeMalloc(l->entryCount * sizeof(const char*));
                repeat(l->entryCount, j) {
                    Reader_readString(&reader, dw, &l->languages[i].entries[j]);
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

void LANG_free(LangChunk *l) {
    if (l == NULL) {
        return;
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
}