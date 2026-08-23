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
        logError("[TAGS_parse] Invalid chunk offset/length: offset=%zu length=%zu file_size=%zu\n",
                 chunk.offset, chunk.length, dw->file_size);
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

    read(&t->count, UInt32);

    if (t->count > 0U) {
        t->strings = (char **)safeCalloc(t->count, sizeof(char *));

        for (uint32_t i = 0; i < t->count; ++i) {
            readString((const char**)&t->strings[i], dw);
        }
    }

    read(&t->asset_tag_count, UInt32);

    if (t->asset_tag_count > 0U) {
        uint32_t *asset_record_offsets = (uint32_t *)safeCalloc(t->asset_tag_count, sizeof(uint32_t));

        for (uint32_t i = 0; i < t->asset_tag_count; ++i) {
            read(&asset_record_offsets[i], UInt32);
        }

        t->asset_tags = (AssetTagEntry *)safeCalloc(t->asset_tag_count, sizeof(AssetTagEntry));

        for (uint32_t i = 0; i < t->asset_tag_count; ++i) {
            const uint32_t absolute_offset = asset_record_offsets[i];
            if (absolute_offset == 0U) {
                continue;
            }

            if (absolute_offset < chunk.offset || absolute_offset + 8U > chunk.offset + chunk.length) {
                logWarn("[TAGS_parse] Asset tag record pointer out of range at index=%u offset=%u chunk_offset=%zu chunk_length=%u\n",
                        i, absolute_offset, chunk.offset, chunk.length);
                continue;
            }

            const uint8_t *record = dw->file_data + absolute_offset;
            const uint32_t id = read_u32_le_at(record, dw->file_size, 0U);
            const uint32_t tag_count = read_u32_le_at(record, dw->file_size, 4U);

            t->asset_tags[i].id = id;
            t->asset_tags[i].tag_count = tag_count;
            t->asset_tags[i].tags = NULL;

            if (tag_count > 0U) {
                if (absolute_offset + 8U + tag_count * sizeof(uint32_t) > chunk.offset + chunk.length) {
                    logWarn("[TAGS_parse] Asset tag record exceeds chunk bounds at index=%u offset=%u tag_count=%u\n",
                            i, absolute_offset, tag_count);
                    t->asset_tags[i].tag_count = 0U;
                } else {
                    t->asset_tags[i].tags = (char **)safeCalloc(tag_count, sizeof(char *));

                    for (uint32_t j = 0; j < tag_count; ++j) {
                        const uint8_t *tag_slot = record + 8U + j * sizeof(uint32_t);
                        uint32_t tag_offset = read_u32_le_at(tag_slot, dw->file_size, 0U);

                        if (tag_offset == 0U) {
                            t->asset_tags[i].tags[j] = NULL;
                            continue;
                        }

                        Reader tag_reader;
                        Reader_init(&tag_reader, tag_slot, sizeof(uint32_t), absolute_offset + 8U + j * sizeof(uint32_t), "TAGS tag");
                        if (Reader_readString(&tag_reader, dw, (const char**)&t->asset_tags[i].tags[j]) != 0) {
                            logWarn("[TAGS_parse] Failed to read tag string at asset index=%u tag index=%u offset=%u\n",
                                    i, j, tag_offset);
                            t->asset_tags[i].tags[j] = NULL;
                        }
                    }
                }
            }
        }

        free(asset_record_offsets);
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
