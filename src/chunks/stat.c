#include "common.h"
#include "../strings.h"

int STAT_free(StatChunk *s);

static void STAT_free_event(StatEvent *event) {
    if (event == NULL) {
        return;
    }

    free(event->name);
    event->name = NULL;
    free(event->version);
    event->version = NULL;

    free(event->fields);
    event->fields = NULL;
    event->fieldCount = 0;
}

static void STAT_free_field(StatEventField *field) {
    if (field == NULL) {
        return;
    }

    free(field->name);
    field->name = NULL;
}

static int STAT_read_string(Reader *reader, DataWin *dw, char **out) {
    if (reader == NULL || dw == NULL || out == NULL) {
        return -1;
    }

    uint32_t offset = 0;
    if (Reader_readUInt32(reader, &offset) != 0) {
        return -1;
    }

    if (offset == 0U) {
        *out = NULL;
        return 0;
    }

    const char *source = get_string(dw, offset);
    if (source == NULL) {
        *out = NULL;
        return 0;
    }

    size_t len = strlen(source);
    char *copy = (char *)malloc(len + 1U);
    if (copy == NULL) {
        *out = NULL;
        return -1;
    }

    memcpy(copy, source, len + 1U);
    *out = copy;
    return 0;
}

static int STAT_parse_string_blob(Reader *reader, DataWin *dw, char **out) {
    if (reader == NULL || dw == NULL || out == NULL) {
        return -1;
    }

    uint32_t length = 0;
    if (Reader_readUInt32(reader, &length) != 0) {
        return -1;
    }

    if (length == 0U) {
        *out = NULL;
        return 0;
    }

    if (reader->cursor + length > reader->size) {
        return -1;
    }

    char *copy = (char *)malloc((size_t)length + 1U);
    if (copy == NULL) {
        *out = NULL;
        return -1;
    }

    memcpy(copy, reader->data + reader->cursor, length);
    copy[length] = '\0';
    reader->cursor += length;

    *out = copy;
    return 0;
}

static int STAT_read_prefixed_utf8(Reader *reader, DataWin *dw, char **out) {
    if (reader == NULL || dw == NULL || out == NULL) {
        return -1;
    }

    uint32_t length = 0;
    if (Reader_readUInt32(reader, &length) != 0) {
        return -1;
    }

    if (length == 0U) {
        *out = NULL;
        return 0;
    }

    if (reader->cursor + length > reader->size) {
        return -1;
    }

    char *copy = (char *)malloc((size_t)length + 1U);
    if (copy == NULL) {
        *out = NULL;
        return -1;
    }

    memcpy(copy, reader->data + reader->cursor, length);
    copy[length] = '\0';
    reader->cursor += length;

    *out = copy;
    return 0;
}

static int STAT_parse_event(Reader *reader, DataWin *dw, StatEvent *event) {
    memset(event, 0, sizeof(*event));

    if (STAT_read_prefixed_utf8(reader, dw, &event->name) != 0) {
        return -1;
    }
    if (STAT_read_prefixed_utf8(reader, dw, &event->version) != 0) {
        return -1;
    }
    if (Reader_readInt32(reader, &event->latency) != 0) {
        return -1;
    }
    if (Reader_readInt32(reader, &event->priority) != 0) {
        return -1;
    }
    if (Reader_readInt32(reader, &event->enabled) != 0) {
        return -1;
    }
    if (Reader_readInt32(reader, &event->populationSampleRate) != 0) {
        return -1;
    }
    if (Reader_readUInt32(reader, &event->id) != 0) {
        return -1;
    }
    if (Reader_readInt32(reader, &event->partCVersion) != 0) {
        return -1;
    }
    if (Reader_readUInt32(reader, &event->fieldCount) != 0) {
        return -1;
    }

    if (event->fieldCount > 0U) {
        event->fields = (StatEventField *)safeCalloc(event->fieldCount, sizeof(StatEventField));
        if (event->fields == NULL) {
            return -1;
        }

        for (uint32_t i = 0; i < event->fieldCount; ++i) {
            if (Reader_readInt32(reader, &event->fields[i].type) != 0) {
                return -1;
            }

            if (STAT_read_prefixed_utf8(reader, dw, &event->fields[i].name) != 0) {
                return -1;
            }
        }
    }

    return 0;
}

static int STAT_parse_provider_defaults(Reader *reader, DataWin *dw, StatChunk *stat) {
    memset(stat, 0, sizeof(*stat));

    if (STAT_read_string(reader, dw, &stat->providerName) != 0) {
        return -1;
    }

    if (Reader_readBytes(reader, stat->providerGuid, 16U) != 0) {
        return -1;
    }

    if (Reader_readInt32(reader, &stat->providerLatency) != 0) {
        return -1;
    }
    if (Reader_readInt32(reader, &stat->providerPriority) != 0) {
        return -1;
    }
    if (Reader_readInt32(reader, &stat->providerEnabled) != 0) {
        return -1;
    }

    uint32_t populationCount = 0;
    if (Reader_readUInt32(reader, &populationCount) != 0) {
        return -1;
    }
    stat->populationSampleRateCount = populationCount;

    if (populationCount > 0U) {
        stat->populationSampleRates = (int32_t *)safeCalloc(populationCount, sizeof(int32_t));
        if (stat->populationSampleRates == NULL) {
            return -1;
        }
        for (uint32_t i = 0; i < populationCount; ++i) {
            if (Reader_readInt32(reader, &stat->populationSampleRates[i]) != 0) {
                return -1;
            }
        }
    }

    return 0;
}

int STAT_parse(DataWin *dw) {
    Chunk chunk = {0};
    StatChunk *s = &dw->stat;

    memset(s, 0, sizeof(*s));

    if (get_chunk(dw, "STAT", &chunk) != 0) {
        return 0;
    }

    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re;
    Reader_init(&re, base, chunk.length, chunk.offset, "STAT");

    if (STAT_parse_provider_defaults(&re, dw, s) != 0) {
        STAT_free(s);
        return -1;
    }

    if (Reader_readUInt32(&re, &s->eventCount) != 0) {
        STAT_free(s);
        return -1;
    }

    if (s->eventCount > 0U) {
        s->events = (StatEvent *)safeCalloc(s->eventCount, sizeof(StatEvent));
        if (s->events == NULL) {
            STAT_free(s);
            return -1;
        }

        for (uint32_t i = 0; i < s->eventCount; ++i) {
            if (STAT_parse_event(&re, dw, &s->events[i]) != 0) {
                STAT_free(s);
                return -1;
            }
        }
    }

    return 0;
}

int STAT_free(StatChunk *s) {
    if (s == NULL) {
        return -1;
    }

    free(s->providerName);
    s->providerName = NULL;
    free(s->populationSampleRates);
    s->populationSampleRates = NULL;
    s->populationSampleRateCount = 0;

    if (s->events != NULL) {
        for (uint32_t i = 0; i < s->eventCount; ++i) {
            STAT_free_event(&s->events[i]);
        }
        free(s->events);
        s->events = NULL;
    }
    s->eventCount = 0;
    return 0;
}
