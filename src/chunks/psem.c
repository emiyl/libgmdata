#include "common.h"

int PSEM_free(PsemChunk *p);

static int PSEM_parse_emitter(Reader *reader, DataWin *dw, ParticleEmitter *emitter) {
    if (emitter == NULL) {
        return -1;
    }

    memset(emitter, 0, sizeof(*emitter));

    readString((const char **)&emitter->name, dw);
    read(&emitter->enabled, Bool32);
    read(&emitter->mode, Int32);
    read(&emitter->emit_count, Float32);
    read(&emitter->emit_relative, Bool32);
    read(&emitter->delay_min, Float32);
    read(&emitter->delay_max, Float32);
    read(&emitter->delay_unit, Int32);
    read(&emitter->interval_min, Float32);
    read(&emitter->interval_max, Float32);
    read(&emitter->interval_unit, Int32);
    read(&emitter->distribution, Int32);
    read(&emitter->shape, Int32);
    read(&emitter->region_x, Float32);
    read(&emitter->region_y, Float32);
    read(&emitter->region_width, Float32);
    read(&emitter->region_height, Float32);
    read(&emitter->rotation, Float32);
    read(&emitter->sprite_id, UInt32);
    read(&emitter->texture_enum, Int32);
    read(&emitter->frame_index, Float32);
    read(&emitter->animate, Bool32);
    read(&emitter->stretch, Bool32);
    read(&emitter->is_random, Bool32);
    read(&emitter->start_color, UInt32);
    read(&emitter->mid_color, UInt32);
    read(&emitter->end_color, UInt32);
    read(&emitter->additive_blend, Bool32);
    read(&emitter->lifetime_min, Float32);
    read(&emitter->lifetime_max, Float32);
    read(&emitter->scale_x, Float32);
    read(&emitter->scale_y, Float32);
    read(&emitter->size_min_x, Float32);
    read(&emitter->size_max_x, Float32);
    read(&emitter->size_min_y, Float32);
    read(&emitter->size_max_y, Float32);
    read(&emitter->size_increase_x, Float32);
    read(&emitter->size_increase_y, Float32);
    read(&emitter->size_wiggle_x, Float32);
    read(&emitter->size_wiggle_y, Float32);
    read(&emitter->speed_min, Float32);
    read(&emitter->speed_max, Float32);
    read(&emitter->speed_increase, Float32);
    read(&emitter->speed_wiggle, Float32);
    read(&emitter->gravity_force, Float32);
    read(&emitter->gravity_direction, Float32);
    read(&emitter->direction_min, Float32);
    read(&emitter->direction_max, Float32);
    read(&emitter->direction_increase, Float32);
    read(&emitter->direction_wiggle, Float32);
    read(&emitter->orientation_min, Float32);
    read(&emitter->orientation_max, Float32);
    read(&emitter->orientation_increase, Float32);
    read(&emitter->orientation_wiggle, Float32);
    read(&emitter->orientation_relative, Bool32);
    read(&emitter->spawn_on_death_id, Int32);
    read(&emitter->spawn_on_death_count, Int32);
    read(&emitter->spawn_on_update_id, Int32);
    read(&emitter->spawn_on_update_count, Int32);

    return 0;
}

int PSEM_parse(DataWin *dw) {
    Chunk chunk = {0};
    PsemChunk *p = &dw->psem;

    memset(p, 0, sizeof(*p));

    if (get_chunk(dw, "PSEM", &chunk) != 0) {
        p->count = 0;
        p->items = NULL;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "PSEM");

    while (reader->cursor % 4U != 0U) {
        uint8_t pad = 0;
        read(&pad, UInt8);
        if (pad != 0U) {
            logWarn("[PSEM_parse] Non-zero padding byte while aligning version\n");
        }
    }

    uint32_t version = 0;
    read(&version, UInt32);
    if (version != 1U) {
        logWarn("[PSEM_parse] Unexpected version %u\n", version);
    }

    read(&p->count, UInt32);

    p->items = NULL;
    if (p->count == 0U) {
        return 0;
    }

    p->items = (ParticleEmitter *)safeCalloc(p->count, sizeof(ParticleEmitter));
    if (p->items == NULL) {
        p->count = 0;
        return -1;
    }

    repeat(p->count, i) {
        if (PSEM_parse_emitter(reader, dw, &p->items[i]) != 0) {
            PSEM_free(p);
            return -1;
        }
    }

    return 0;
}

int PSEM_free(PsemChunk *p) {
    if (p == NULL) {
        return -1;
    }
    if (p->items != NULL) {
        repeat(p->count, i) {
            if (p->items[i].name != NULL) {
                free((void *)p->items[i].name);
                p->items[i].name = NULL;
            }
        }
        free(p->items);
        p->items = NULL;
    }
    p->count = 0;
    return 0;
}
