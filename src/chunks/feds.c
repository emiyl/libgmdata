#include "common.h"

static int FEDS_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData);
static int FEDS_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData);

static int FilterEffect_parse(Reader *reader, DataWin *dw, FilterEffect *effect) {
    memset(effect, 0, sizeof(*effect));
    effect->present = true;
    readString(&effect->name, dw);
    readString(&effect->value, dw);
    return 0;
}

static int FEDS_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)extraData;
    return FilterEffect_parse(reader, dw, (FilterEffect *)out);
}

static int FEDS_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)reader; (void)dw; (void)extraData;
    FilterEffect *effect = (FilterEffect *)out;
    memset(effect, 0, sizeof(*effect));
    effect->present = false;
    return 0;
}

int FEDS_parse(DataWin *dw) {
    Chunk chunk = {0};
    FedsChunk *f = &dw->feds;

    if (get_chunk(dw, "FEDS", &chunk) != 0) {
        f->count = 0;
        f->effects = NULL;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "FEDS");

    while (reader->cursor % 4U != 0U) {
        uint8_t pad = 0;
        read(&pad, UInt8);
        if (pad != 0U) {
            logWarn("[FEDS_parse] Non-zero padding byte found during version alignment\n");
        }
    }

    uint32_t version = 0;
    read(&version, UInt32);
    if (version != 1U) {
        logWarn("[FEDS_parse] Unexpected FEDS version %u (expected 1)\n", version);
    }

    return Reader_readAndParsePointerTable(
        reader, dw,
        (void **)&f->effects, NULL,
        &f->count, sizeof(FilterEffect),
        FEDS_pointerTable_parse,
        FEDS_pointerTable_missingHandler,
        NULL
    );
}

static void FEDS_free_effect(FilterEffect *effect) {
    if (effect == NULL) return;
    free((void *)effect->name);
    effect->name = NULL;
    free((void *)effect->value);
    effect->value = NULL;
    effect->present = false;
}

int FEDS_free(FedsChunk *f) {
    if (f == NULL) return -1;
    if (f->effects != NULL) {
        for (uint32_t i = 0; i < f->count; ++i) {
            FEDS_free_effect(&f->effects[i]);
        }
        free(f->effects);
    }
    f->effects = NULL;
    f->count = 0;
    return 0;
}
