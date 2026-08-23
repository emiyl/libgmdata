#include "common.h"

int PSYS_free(PsysChunk *p);

static int PSYS_parse_system(Reader *reader, DataWin *dw, ParticleSystem *system) {
    if (system == NULL) {
        return -1;
    }

    memset(system, 0, sizeof(*system));

    readString((const char **)&system->name, dw);
    read(&system->origin_x, Int32);
    read(&system->origin_y, Int32);
    read(&system->draw_order, Int32);
    read(&system->global_space_particles, Bool32);
    read(&system->emitter_count, UInt32);

    system->emitters = NULL;
    if (system->emitter_count > 0U) {
        system->emitters = (ParticleEmitter *)safeCalloc(system->emitter_count, sizeof(ParticleEmitter));
        if (system->emitters == NULL) {
            return -1;
        }

        repeat(system->emitter_count, i) {
            uint32_t emitter_index = 0;
            if (Reader_readUInt32(reader, &emitter_index) != 0) {
                free(system->emitters);
                system->emitters = NULL;
                return -1;
            }

            if (emitter_index < dw->psem.count && dw->psem.items != NULL) {
                system->emitters[i] = dw->psem.items[emitter_index];
            } else {
                memset(&system->emitters[i], 0, sizeof(system->emitters[i]));
            }
        }
    }

    return 0;
}

int PSYS_parse(DataWin *dw) {
    Chunk chunk = {0};
    PsysChunk *p = &dw->psys;

    memset(p, 0, sizeof(*p));

    if (get_chunk(dw, "PSYS", &chunk) != 0) {
        p->count = 0;
        p->items = NULL;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "PSYS");

    while (reader->cursor % 4U != 0U) {
        uint8_t pad = 0;
        read(&pad, UInt8);
        if (pad != 0U) {
            logWarn("[PSYS_parse] Non-zero padding byte while aligning version\n");
        }
    }

    uint32_t version = 0;
    if (Reader_readUInt32(reader, &version) != 0 || version != 1U) {
        logWarn("[PSYS_parse] Unexpected version %u\n", version);
    }

    read(&p->count, UInt32);

    p->items = NULL;
    if (p->count == 0U) {
        return 0;
    }

    p->items = (ParticleSystem *)safeCalloc(p->count, sizeof(ParticleSystem));
    if (p->items == NULL) {
        p->count = 0;
        return -1;
    }

    repeat(p->count, i) {
        if (PSYS_parse_system(reader, dw, &p->items[i]) != 0) {
            PSYS_free(p);
            return -1;
        }
    }

    return 0;
}

int PSYS_free(PsysChunk *p) {
    if (p == NULL) {
        return -1;
    }
    if (p->items != NULL) {
        repeat(p->count, i) {
            if (p->items[i].name != NULL) {
                free((void *)p->items[i].name);
                p->items[i].name = NULL;
            }
            if (p->items[i].emitters != NULL) {
                free(p->items[i].emitters);
                p->items[i].emitters = NULL;
            }
            p->items[i].emitter_count = 0;
        }
        free(p->items);
        p->items = NULL;
    }
    p->count = 0;
    return 0;
}
