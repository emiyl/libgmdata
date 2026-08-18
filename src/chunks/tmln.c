#include "common.h"

int Timeline_parse(Reader *reader, DataWin *dw, Timeline *timeline);
int TimelineMoment_parse(Reader *reader, DataWin *dw, TimelineMoment *moment);
int EventAction_parse(Reader *reader, DataWin *dw, EventAction *action);

int TMLN_parse(DataWin *dw) {
    Chunk chunk = {0};
    TmlnChunk *t = &dw->tmln;

    if (find_chunk(dw, "TMLN", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "TMLN");

    uint32_t count;
    uint32_t *ptrs;
    Reader_readPointerTable(&reader, &ptrs, &count);
    t->count = count;

    if (count == 0) {
        t->timelines = NULL;
        free(ptrs);
        return 0;
    }

    t->timelines = (Timeline*)safeCalloc(count, sizeof(Timeline));
    repeat(count, i) {
        if (ptrs[i] == 0) continue;
        Reader_seek(&reader, ptrs[i]);
        if (Timeline_parse(&reader, dw, &t->timelines[i]) != 0) {
            free(ptrs);
            free(t->timelines);
            return -1;
        }
    }

    free(ptrs);
    return 0;
}

int Timeline_parse(Reader *reader, DataWin *dw, Timeline *tl) {
    tl->present = true;
    Reader_readString(reader, dw, &tl->name);
    Reader_readUInt32(reader, &tl->momentCount);

    if (tl->momentCount == 0) {
        tl->moments = NULL;
        return 0;
    }

    tl->moments = (TimelineMoment*)safeMalloc(tl->momentCount * sizeof(TimelineMoment));
    repeat(tl->momentCount, i) {
        uint32_t *eventPtrs = (uint32_t *)safeMalloc(tl->momentCount * sizeof(uint32_t));

        // Pass 1: Read step + event pointer pairs
        repeat(tl->momentCount, j) {
            Reader_readUInt32(reader, &eventPtrs[j]);
            Reader_readUInt32(reader, &eventPtrs[j]);
        }

        // Pass 2: Parse event action lists
        {
        repeat(tl->momentCount, j) {
            Reader_seek(reader, eventPtrs[j]);
            uint32_t count;
            uint32_t *ptrs;
            Reader_readPointerTable(reader, &ptrs, &count);
            tl->moments[j].actionCount = count;
            
            if (count == 0) {
                free(ptrs);
                tl->moments[j].actions = NULL;
            }

            tl->moments[j].actions = (EventAction *)safeMalloc(count * sizeof(EventAction));
            repeat(count, k) {
                Reader_seek(reader, ptrs[k]);
                EventAction_parse(reader, dw, &tl->moments[j].actions[k]);
            }
            free(ptrs);
        }
        }
    }

    return 0;
}

int EventAction_parse(Reader *reader, DataWin *dw, EventAction *action) {
    Reader_readUInt32(reader, &action->libID);
    Reader_readUInt32(reader, &action->id);
    Reader_readUInt32(reader, &action->kind);
    Reader_readBool32(reader, &action->useRelative);
    Reader_readBool32(reader, &action->isQuestion);
    Reader_readBool32(reader, &action->useApplyTo);
    Reader_readUInt32(reader, &action->exeType);
    Reader_readString(reader, dw, &action->actionName);
    Reader_readInt32(reader, &action->codeId);
    Reader_readUInt32(reader, &action->argumentCount);
    Reader_readInt32(reader, &action->who);
    Reader_readBool32(reader, &action->relative);
    Reader_readBool32(reader, &action->isNot);
    Reader_readUInt32(reader, &action->unknownAlwaysZero);
    return 0;
}

int EventAction_free(EventAction *action) {
    action->actionName = NULL;
    return 0;
}

int TimelineMoment_free(TimelineMoment *moment) {
    repeat(moment->actionCount, i) {
        EventAction_free(&moment->actions[i]);
    }
    free(moment->actions);
    moment->actions = NULL;
    moment->actionCount = 0;
    return 0;
}

int Timeline_free(Timeline *timeline) {
    free((void *)timeline->name);
    timeline->name = NULL;

    repeat(timeline->momentCount, i) {
        TimelineMoment_free(&timeline->moments[i]);
    }
    free(timeline->moments);
    timeline->moments = NULL;
    timeline->momentCount = 0;
    return 0;
}

int TMLN_free(TmlnChunk *tmln) {
    repeat(tmln->count, i) {
        Timeline_free(&tmln->timelines[i]);
    }
    free(tmln->timelines);
    tmln->timelines = NULL;
    tmln->count = 0;
    return 0;
}