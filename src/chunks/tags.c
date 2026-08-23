#include "common.h"

int TAGS_free(TagsChunk *t);

int TAGS_parse(DataWin *dw) {
    Chunk chunk = {0};
    TagsChunk *t = &dw->tags;

    t->count = 0;
    t->strings = NULL;
    t->asset_tag_count = 0;
    t->asset_tags = NULL;

    if (get_chunk(dw, "TAGS", &chunk) != 0) {
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "TAGS");

    while (reader->cursor % 4U != 0U) {
        uint8_t pad = 0;
        read(&pad, UInt8);
        if (pad != 0U) {
            logWarn("[TAGS_parse] Non-zero padding byte while aligning version\n");
        }
    }

    uint32_t version = 0;
    if (Reader_readUInt32(reader, &version) != 0 || version != 1U) {
        logWarn("[TAGS_parse] Unexpected version %u\n", version);
    }

    t->strings = NULL;
    t->asset_tag_count = 0;
    t->asset_tags = NULL;

    if (Reader_readUInt32(reader, &t->count) != 0) {
        TAGS_free(t);
        return -1;
    }

    if (t->count > 0U) {
        t->strings = (char **)safeCalloc(t->count, sizeof(char *));
        if (t->strings == NULL) {
            t->count = 0;
            return -1;
        }

        for (uint32_t i = 0; i < t->count; ++i) {
            if (Reader_readString(reader, dw, (const char **)&t->strings[i]) != 0) {
                TAGS_free(t);
                return -1;
            }
        }
    }

    if (Reader_readUInt32(reader, &t->asset_tag_count) != 0) {
        TAGS_free(t);
        return -1;
    }

    if (t->asset_tag_count > 0U) {
        t->asset_tags = (AssetTagEntry *)safeCalloc(t->asset_tag_count, sizeof(AssetTagEntry));
        if (t->asset_tags == NULL) {
            TAGS_free(t);
            return -1;
        }

        for (uint32_t i = 0; i < t->asset_tag_count; ++i) {
            uint32_t id = 0;
            uint32_t tag_count = 0;

            if (Reader_readUInt32(reader, &id) != 0) {
                TAGS_free(t);
                return -1;
            }
            if (Reader_readUInt32(reader, &tag_count) != 0) {
                TAGS_free(t);
                return -1;
            }

            t->asset_tags[i].id = id;
            t->asset_tags[i].tag_count = tag_count;
            t->asset_tags[i].tags = NULL;

            if (tag_count > 0U) {
                t->asset_tags[i].tags = (char **)safeCalloc(tag_count, sizeof(char *));
                if (t->asset_tags[i].tags == NULL) {
                    TAGS_free(t);
                    return -1;
                }

                for (uint32_t j = 0; j < tag_count; ++j) {
                    if (Reader_readString(reader, dw, (const char **)&t->asset_tags[i].tags[j]) != 0) {
                        TAGS_free(t);
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}

int TAGS_free(TagsChunk *t) {
    if (t == NULL) {
        return -1;
    }

    if (t->strings != NULL) {
        if ((uintptr_t)t->strings <= 4096U || t->count == 0U) {
            t->strings = NULL;
        } else {
            for (uint32_t i = 0; i < t->count; ++i) {
                if (t->strings[i] != NULL) {
                    free((void *)t->strings[i]);
                    t->strings[i] = NULL;
                }
            }
            free(t->strings);
            t->strings = NULL;
        }
    }

    if (t->asset_tags != NULL) {
        if ((uintptr_t)t->asset_tags <= 4096U || t->asset_tag_count == 0U) {
            t->asset_tags = NULL;
        } else {
            for (uint32_t i = 0; i < t->asset_tag_count; ++i) {
                if (t->asset_tags[i].tags != NULL) {
                    for (uint32_t j = 0; j < t->asset_tags[i].tag_count; ++j) {
                        if (t->asset_tags[i].tags[j] != NULL) {
                            free((void *)t->asset_tags[i].tags[j]);
                            t->asset_tags[i].tags[j] = NULL;
                        }
                    }
                    free(t->asset_tags[i].tags);
                    t->asset_tags[i].tags = NULL;
                }
            }
            free(t->asset_tags);
            t->asset_tags = NULL;
        }
    }

    t->count = 0;
    t->asset_tag_count = 0;
    return 0;
}
