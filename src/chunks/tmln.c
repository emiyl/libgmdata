#include "common.h"

static int TMLN_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int TMLN_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
int TMLN_parse(DataWin *dw) {
    Chunk chunk = {0};
    TmlnChunk *t = &dw->tmln;

    if (get_chunk(dw, "TMLN", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "TMLN");

    return Reader_readAndParsePointerTable(
        reader, dw,
        (void **)&t->timelines, NULL,
        &t->count, sizeof(Timeline),
        TMLN_pointerTable_parse,
        TMLN_pointerTable_missingHandler,
        NULL
    );
}

static int Timeline_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int Timeline_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
static int Timeline_parse(Reader *reader, DataWin *dw, Timeline *tl) {
    tl->present = true;
    readString(&tl->name, dw);
    read(&tl->momentCount, UInt32);

    if (tl->momentCount == 0) {
        tl->moments = NULL;
        return 0;
    }

    tl->moments = (TimelineMoment*)safeMalloc(tl->momentCount * sizeof(TimelineMoment));
    repeat(tl->momentCount, i) {
        uint32_t *eventPtrs = (uint32_t *)safeMalloc(tl->momentCount * sizeof(uint32_t));

        // Pass 1: Read step + event pointer pairs
        repeat(tl->momentCount, j) {
            read(&tl->moments[j].step, UInt32);
            read(&eventPtrs[j], UInt32);
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
            
            int result = Reader_parsePointerTable(
                reader, dw,
                ptrs, count,
                (void **)&tl->moments[j].actions, sizeof(EventAction),
                NULL,
                Timeline_pointerTable_parse,
                Timeline_pointerTable_missingHandler,
                NULL
            );

            free(ptrs);
            if (result != 0) {
                free(eventPtrs);
                return result;
            }
        }
        }
    }

    return 0;
}

// Used in objt.c too
int EventAction_parse(Reader *reader, DataWin *dw, EventAction *action) {
    read(&action->libID, UInt32);
    read(&action->id, UInt32);
    read(&action->kind, UInt32);
    read(&action->useRelative, Bool32);
    read(&action->isQuestion, Bool32);
    read(&action->useApplyTo, Bool32);
    read(&action->exeType, UInt32);
    readString(&action->actionName, dw);
    read(&action->codeId, Int32);
    read(&action->argumentCount, UInt32);
    read(&action->who, Int32);
    read(&action->relative, Bool32);
    read(&action->isNot, Bool32);
    read(&action->unknownAlwaysZero, UInt32);
    
    return 0;
}

static int TMLN_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)extraData; // Unused parameter
    return Timeline_parse(reader, dw, (Timeline *)out);
}

static int TMLN_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[TMLN_pointerTable_missingHandler] Timeline pointer is missing, initializing default values.\n");

    Timeline *timeline = (Timeline *)out;
    timeline->present = false;
    timeline->name = NULL;
    timeline->momentCount = 0;
    timeline->moments = NULL;

    return 0;
}

static int Timeline_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)extraData; // Unused parameter
    return EventAction_parse(reader, dw, (EventAction *)out);
}

static int Timeline_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[Timeline_pointerTable_missingHandler] Timeline moment pointer is missing, initializing default values.\n");

    TimelineMoment *moment = (TimelineMoment *)out;
    moment->actionCount = 0;
    moment->actions = NULL;

    return 0;
}

static int EventAction_free(EventAction *action) {
    action->actionName = NULL;
    return 0;
}

static int TimelineMoment_free(TimelineMoment *moment) {
    repeat(moment->actionCount, i) {
        EventAction_free(&moment->actions[i]);
    }
    free(moment->actions);
    moment->actions = NULL;
    moment->actionCount = 0;
    return 0;
}

static int Timeline_free(Timeline *timeline) {
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