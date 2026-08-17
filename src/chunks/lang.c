#include "lang.h"

int LANG_Parse(DataWin *dw) {
    Chunk chunk = {0};
    Lang *l = &dw->lang;

    if (find_chunk(dw, "LANG", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    reader_init(&reader, base, chunk.length);

    reader_read_u32(&reader, &l->unknown1);
    reader_read_u32(&reader, &l->languageCount);
    reader_read_u32(&reader, &l->entryCount);

    // Entry IDs
    if (l->entryCount > 0) {
        l->entryIds = (const char **)malloc(l->entryCount * sizeof(const char*));
        repeat(l->entryCount, i) {
            reader_read_string(&reader, dw, &l->entryIds[i]);
        }
    } else {
        l->entryIds = NULL;
    }

    // Languages
    if (l->languageCount > 0) {
        l->languages = (Language *)malloc(l->languageCount * sizeof(Language));
        repeat(l->languageCount, i) {
            reader_read_string(&reader, dw, &l->languages[i].name);
            reader_read_string(&reader, dw, &l->languages[i].region);
            l->languages[i].entryCount = l->entryCount;
            if (l->entryCount > 0) {
                l->languages[i].entries = (const char **)malloc(l->entryCount * sizeof(const char*));
                repeat(l->entryCount, j) {
                    reader_read_string(&reader, dw, &l->languages[i].entries[j]);
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