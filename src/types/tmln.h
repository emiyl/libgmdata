#ifndef TMLN_TYPES_H
#define TMLN_TYPES_H

#include <stdint.h>
#include "event_action.h"

typedef struct {
    uint32_t step;
    uint32_t actionCount;
    EventAction* actions;
} TimelineMoment;

typedef struct {
    bool present;
    const char* name;
    uint32_t momentCount;
    TimelineMoment* moments;
} Timeline;

typedef struct {
    uint32_t count;
    Timeline* timelines;
} TmlnChunk;

#endif