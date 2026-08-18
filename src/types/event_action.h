#ifndef EVENT_ACTION_TYPES_H
#define EVENT_ACTION_TYPES_H

#include <stdint.h>

// shared by TMLN and OBJT
typedef struct {
    uint32_t libID;
    uint32_t id;
    uint32_t kind;
    bool useRelative;
    bool isQuestion;
    bool useApplyTo;
    uint32_t exeType;
    const char* actionName;
    int32_t codeId;
    uint32_t argumentCount;
    int32_t who;
    bool relative;
    bool isNot;
    uint32_t unknownAlwaysZero;
} EventAction;

#endif