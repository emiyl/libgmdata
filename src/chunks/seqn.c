#include "common.h"

int SEQN_free(SeqnChunk *s);

static int SEQN_parse_channel(Reader *reader, DataWin *dw, SequenceChannel *channel) {
    (void)dw;

    read(&channel->type, UInt32);
    read(&channel->index, UInt32);
    read(&channel->value_count, UInt32);

    channel->values = NULL;
    if (channel->value_count > 0U) {
        channel->values = (float *)safeCalloc(channel->value_count, sizeof(float));
        if (channel->values == NULL) {
            return -1;
        }

        repeat(channel->value_count, i) {
            if (Reader_readFloat32(reader, &channel->values[i]) != 0) {
                free(channel->values);
                channel->values = NULL;
                return -1;
            }
        }
    }

    return 0;
}

static int SEQN_parse_keyframe(Reader *reader, DataWin *dw, SequenceKeyframe *keyframe) {
    read(&keyframe->key, Float32);
    read(&keyframe->length, Float32);
    read(&keyframe->stretch, Bool32);
    read(&keyframe->disabled, Bool32);
    read(&keyframe->channel_count, UInt32);

    keyframe->channels = NULL;
    if (keyframe->channel_count > 0U) {
        keyframe->channels = (SequenceChannel *)safeCalloc(keyframe->channel_count, sizeof(SequenceChannel));
        if (keyframe->channels == NULL) {
            return -1;
        }

        repeat(keyframe->channel_count, i) {
            if (SEQN_parse_channel(reader, dw, &keyframe->channels[i]) != 0) {
                free(keyframe->channels);
                keyframe->channels = NULL;
                return -1;
            }
        }
    }

    return 0;
}

static int SEQN_parse_track(Reader *reader, DataWin *dw, SequenceTrack *track) {
    read(&track->track_type, UInt32);
    read(&track->channel_count, UInt32);

    track->channels = NULL;
    if (track->channel_count > 0U) {
        track->channels = (SequenceChannel *)safeCalloc(track->channel_count, sizeof(SequenceChannel));
        if (track->channels == NULL) {
            return -1;
        }

        repeat(track->channel_count, i) {
            if (SEQN_parse_channel(reader, dw, &track->channels[i]) != 0) {
                free(track->channels);
                track->channels = NULL;
                return -1;
            }
        }
    }

    return 0;
}

static void SEQN_free_keyframe(SequenceKeyframe *keyframe) {
    if (keyframe == NULL) {
        return;
    }

    if (keyframe->channels != NULL) {
        repeat(keyframe->channel_count, i) {
            free(keyframe->channels[i].values);
            keyframe->channels[i].values = NULL;
            keyframe->channels[i].value_count = 0;
        }
        free(keyframe->channels);
        keyframe->channels = NULL;
    }

    keyframe->key = 0.0f;
    keyframe->length = 0.0f;
    keyframe->stretch = false;
    keyframe->disabled = false;
    keyframe->channel_count = 0;
}

static void SEQN_free_track(SequenceTrack *track) {
    if (track == NULL) {
        return;
    }

    if (track->channels != NULL) {
        repeat(track->channel_count, i) {
            free(track->channels[i].values);
            track->channels[i].values = NULL;
            track->channels[i].value_count = 0;
        }
        free(track->channels);
        track->channels = NULL;
    }

    track->track_type = 0;
    track->channel_count = 0;
}

static void SEQN_free_sequence(Sequence *sequence) {
    if (sequence == NULL) {
        return;
    }

    free((void *)sequence->name);
    sequence->name = NULL;

    if (sequence->broadcast_messages != NULL) {
        repeat(sequence->broadcast_message_count, i) {
            SEQN_free_keyframe(&sequence->broadcast_messages[i]);
        }
        free(sequence->broadcast_messages);
        sequence->broadcast_messages = NULL;
    }

    if (sequence->tracks != NULL) {
        repeat(sequence->track_count, i) {
            SEQN_free_track(&sequence->tracks[i]);
        }
        free(sequence->tracks);
        sequence->tracks = NULL;
    }

    if (sequence->function_ids != NULL) {
        free(sequence->function_ids);
        sequence->function_ids = NULL;
    }

    if (sequence->moments != NULL) {
        repeat(sequence->moment_count, i) {
            SEQN_free_keyframe(&sequence->moments[i]);
        }
        free(sequence->moments);
        sequence->moments = NULL;
    }

    sequence->playback = 0;
    sequence->playback_speed = 0.0f;
    sequence->playback_speed_type = 0;
    sequence->length = 0.0f;
    sequence->origin_x = 0;
    sequence->origin_y = 0;
    sequence->volume = 0.0f;
    sequence->width = 0.0f;
    sequence->height = 0.0f;
    sequence->broadcast_message_count = 0;
    sequence->track_count = 0;
    sequence->function_id_count = 0;
    sequence->moment_count = 0;
}

static int SEQN_parse_sequence(Reader *reader, DataWin *dw, Sequence *sequence) {
    if (sequence == NULL) {
        return -1;
    }

    readString((const char**)&sequence->name, dw);
    read(&sequence->playback, UInt32);
    read(&sequence->playback_speed, Float32);
    read(&sequence->playback_speed_type, UInt32);
    read(&sequence->length, Float32);
    read(&sequence->origin_x, Int32);
    read(&sequence->origin_y, Int32);
    read(&sequence->volume, Float32);
    read(&sequence->width, Float32);
    read(&sequence->height, Float32);
    read(&sequence->broadcast_message_count, UInt32);
    
    if (sequence->broadcast_message_count > 0U) {
        sequence->broadcast_messages = (SequenceKeyframe *)safeCalloc(sequence->broadcast_message_count, sizeof(SequenceKeyframe));
        if (sequence->broadcast_messages == NULL) {
            return -1;
        }
        repeat(sequence->broadcast_message_count, j) {
            if (SEQN_parse_keyframe(reader, dw, &sequence->broadcast_messages[j]) != 0) {
                return -1;
            }
        }
    }

    read(&sequence->track_count, UInt32);
    
    if (sequence->track_count > 0U) {
        sequence->tracks = (SequenceTrack *)safeCalloc(sequence->track_count, sizeof(SequenceTrack));
        if (sequence->tracks == NULL) {
            return -1;
        }
        repeat(sequence->track_count, j) {
            if (SEQN_parse_track(reader, dw, &sequence->tracks[j]) != 0) {
                return -1;
            }
        }
    }

    read(&sequence->function_id_count, UInt32);
    
    if (sequence->function_id_count > 0U) {
        sequence->function_ids = (SequenceFunctionIdEntry *)safeCalloc(sequence->function_id_count, sizeof(SequenceFunctionIdEntry));
        if (sequence->function_ids == NULL) {
            return -1;
        }
        repeat(sequence->function_id_count, j) {
            SequenceFunctionIdEntry *entry = &sequence->function_ids[j];
            read(&entry->function_id, UInt32);
            read(&entry->value, UInt32);
        }
    }

    read(&sequence->moment_count, UInt32);
    
    if (sequence->moment_count > 0U) {
        sequence->moments = (SequenceKeyframe *)safeCalloc(sequence->moment_count, sizeof(SequenceKeyframe));
        if (sequence->moments == NULL) {
            return -1;
        }
        repeat(sequence->moment_count, j) {
            if (SEQN_parse_keyframe(reader, dw, &sequence->moments[j]) != 0) {
                return -1;
            }
        }
    }

    return 0;
}

int SEQN_parse(DataWin *dw) {
    Chunk chunk = {0};
    SeqnChunk *s = &dw->seqn;

    memset(s, 0, sizeof(*s));

    if (get_chunk(dw, "SEQN", &chunk) != 0) {
        s->count = 0;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "SEQN");

    while (reader->cursor % 4U != 0U) {
        uint8_t pad = 0;
        read(&pad, UInt8);
        if (pad != 0U) {
            logWarn("[SEQN_parse] Non-zero padding byte while aligning version\n");
        }
    }

    uint32_t version = 0;
    if (Reader_readUInt32(reader, &version) != 0 || version != 1U) {
        logWarn("[SEQN_parse] Unexpected version %u\n", version);
    }

    read(&s->count, UInt32);

    s->items = NULL;
    if (s->count > 0U) {
        s->items = (Sequence *)safeCalloc(s->count, sizeof(Sequence));
        if (s->items == NULL) {
            s->count = 0;
            return -1;
        }

        for (uint32_t i = 0; i < s->count; ++i) {
            if (SEQN_parse_sequence(reader, dw, &s->items[i]) != 0) {
                SEQN_free(s);
                return -1;
            }
        }
    }

    return 0;
}

int SEQN_free(SeqnChunk *s) {
    if (s == NULL) {
        return -1;
    }

    if (s->items != NULL) {
        repeat(s->count, i) {
            SEQN_free_sequence(&s->items[i]);
        }
        free(s->items);
        s->items = NULL;
    }

    s->count = 0;
    return 0;
}
