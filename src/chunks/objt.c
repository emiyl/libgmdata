#include "common.h"

static int OBJT_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int OBJT_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
int OBJT_parse(DataWin *dw) {
    logDebug("[OBJT_parse] Parsing OBJT chunk...\n");

    Chunk chunk = {0};
    ObjtChunk *b = &dw->objt;

    if (get_chunk(dw, "OBJT", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    logDebug("[OBJT_parse] Found OBJT chunk at offset %zu, length %zu\n", chunk.offset, chunk.length);

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "OBJT");

    logDebug("[OBJT_parse] Reading pointer table for %u game objects...\n", b->count);

    uint32_t *ptrs;
    if (Reader_readPointerTable(reader, &ptrs, &b->count) != 0) return -1;

    if (b->count == 0) {
        b->objects = NULL;
        free(ptrs);
        return 0;
    }

    logDebug("[OBJT_parse] Parsing %u game objects...\n", b->count);
    
    int result = Reader_parsePointerTable(
        reader, dw,
        ptrs, b->count,
        (void **)&b->objects, sizeof(GameObject),
        NULL,
        OBJT_pointerTable_parse,
        OBJT_pointerTable_missingHandler,
        NULL
    );
    
    free(ptrs);
    return result;
}

static int OBJT_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[OBJT_pointerTable_missingHandler] Missing game object pointer, initializing default GameObject.\n");

    if (out == NULL) {
        logWarn("[OBJT_pointerTable_missingHandler] out is NULL, cannot initialize GameObject.\n");
        return -1;
    }
    
    GameObject *o = (GameObject *)out;
    o->present = false;
    o->name = NULL;
    o->spriteId = -1;
    o->visible = false;
    o->managed = false;
    o->solid = false;
    o->depth = -1;
    o->persistent = false;
    o->parentId = -1;
    o->textureMaskId = -1;
    o->usesPhysics = false;
    o->isSensor = false;
    o->collisionShape = 0;
    o->density = 0.0f;
    o->restitution = 0.0f;
    o->group = 0;
    o->linearDamping = 0.0f;
    o->angularDamping = 0.0f;
    o->physicsVertexCount = 0;
    o->physicsVertices = NULL;

    return 0;
}

static int PhysicsVertex_parse(Reader *reader, PhysicsVertex *vertex);
static int GameObject_parse(Reader *reader, DataWin *dw, GameObject *obj);
static int GameObject_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int GameObject_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
static int OBJT_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)extraData; // Unused parameter

    if (reader == NULL || dw == NULL || out == NULL) {
        logWarn("[OBJT_pointerTable_parse] Invalid parameters: reader=%p, dw=%p, out=%p\n", (void*)reader, (void*)dw, (void*)out);
        return -1;
    }

    logDebug("[OBJT_pointerTable_parse] Parsing GameObject at offset %zu\n", reader->cursor);
    GameObject *obj = (GameObject *)out;
    if (GameObject_parse(reader, dw, obj) != 0) {
        logWarn("[OBJT_pointerTable_parse] Failed to parse GameObject at offset %zu\n", reader->cursor);
        return -1;
    }

    return 0;
}

static int GameObject_parse(Reader *reader, DataWin *dw, GameObject *o) {
    logDebug("[GameObject_parse] Parsing GameObject...\n");
    
    o->present = true;
    readString(&o->name, dw);
    read(&o->spriteId, Int32);
    read(&o->visible, Bool32);
    if (DataWin_isVersionAtLeast(dw, 2022, 5, 0, 0)) {
        read(&o->managed, Bool32);
    } else {
        o->managed = false;
    }
    read(&o->solid, Bool32);
    read(&o->depth, Int32);
    read(&o->persistent, Bool32);
    read(&o->parentId, Int32);
    read(&o->textureMaskId, Int32);
    read(&o->usesPhysics, Bool32);
    read(&o->isSensor, Bool32);
    read(&o->collisionShape, UInt32);
    read(&o->density, Float32);
    read(&o->restitution, Float32);
    read(&o->group, UInt32);
    read(&o->linearDamping, Float32);
    read(&o->angularDamping, Float32);
    read(&o->physicsVertexCount, Int32);

    // WAD8 object records end at physicsVertexCount (no friction/awake/kinematic before the events list)
    if (8 >= dw->gen8.wadVersion) {
        o->friction = 0;
        o->awake = false;
        o->kinematic = false;
    } else {
        read(&o->friction, Float32);
        read(&o->awake, Bool32);
        read(&o->kinematic, Bool32);
    }

    // Physics vertices
    if (o->physicsVertexCount > 0) {
        o->physicsVertices = (PhysicsVertex *)safeMalloc(o->physicsVertexCount * sizeof(PhysicsVertex));
        logDebug("[OBJT_pointerTable_parse] Parsing %u physics vertices for object '%s'\n", o->physicsVertexCount, o->name ? o->name : "(null)");
        repeat(o->physicsVertexCount, i) {
            if (PhysicsVertex_parse(reader, &o->physicsVertices[i]) != 0) {
                logWarn("[OBJT_pointerTable_parse] Failed to parse physics vertex %u for object '%s'\n", i, o->name ? o->name : "(null)");
                free(o->physicsVertices);
                o->physicsVertices = NULL;
                return -1;
            }
        }
    } else {
        o->physicsVertices = NULL;
    }

    uint32_t eventTypeCount;
    uint32_t *eventTypePtrs;
    Reader_readPointerTable(reader, &eventTypePtrs, &eventTypeCount);

    for (uint32_t eventType = 0; eventType < eventTypeCount && eventType < OBJT_EVENT_TYPE_COUNT; eventType++) {
        Reader_seek(reader, eventTypePtrs[eventType]);

        logDebug("[OBJT_pointerTable_parse] Parsing event type %u for object '%s'\n", eventType, o->name ? o->name : "(null)");

        ObjectEventList *eventList = &o->eventLists[eventType];

        // Inner pointer list: events for this type
        uint32_t *eventPtrs;
        Reader_readPointerTable(reader, &eventPtrs, &eventList->eventCount);

        logDebug("[OBJT_pointerTable_parse] Found %u events for event type %u of object '%s'\n", eventList->eventCount, eventType, o->name ? o->name : "(null)");
        
        eventList->events = (ObjectEvent *)safeMalloc(eventList->eventCount * sizeof(ObjectEvent));
        repeat(eventList->eventCount, eventIndex) {
            logDebug("[OBJT_pointerTable_parse] Parsing event %u for event type %u of object '%s'\n", eventIndex, eventType, o->name ? o->name : "(null)");
        
            Reader_seek(reader, eventPtrs[eventIndex]);
            ObjectEvent *event = &eventList->events[eventIndex];
            read(&event->eventSubtype, UInt32);
            
            uint32_t *actionPtrs;
            Reader_readPointerTable(reader, &actionPtrs, &event->actionCount);
            
            if (event->actionCount == 0) {
                free(actionPtrs);
                event->actions = NULL;
            }

            int result = Reader_parsePointerTable(
                reader, dw,
                actionPtrs, event->actionCount,
                (void **)&event->actions, sizeof(EventAction),
                NULL,
                GameObject_pointerTable_parse,
                GameObject_pointerTable_missingHandler,
                NULL
            );

            if (result != 0) {
                logWarn("[OBJT_pointerTable_parse] Failed to parse actions for event type %u, subtype %u of object '%s'\n", eventType, event->eventSubtype, o->name ? o->name : "(null)");
                free(eventList->events);
                eventList->events = NULL;
                free(eventPtrs);
                return -1;
            }

            free(actionPtrs);
        }

        for (uint32_t eventType = eventTypeCount; OBJT_EVENT_TYPE_COUNT > eventType; eventType++) {
            o->eventLists[eventType].eventCount = 0;
            o->eventLists[eventType].events = NULL;
        }

        free(eventPtrs);
    }

    free(eventTypePtrs);
    return 0;
}

int EventAction_parse(Reader *reader, DataWin *dw, EventAction *action); // tmln.c
static int GameObject_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)extraData; // Unused parameter
    return EventAction_parse(reader, dw, (EventAction *)out);
}

static int GameObject_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[GameObject_pointerTable_missingHandler] GameObject pointer is missing, initializing default values.\n");

    if (out == NULL) {
        logWarn("[GameObject_pointerTable_missingHandler] out is NULL, cannot initialize GameObject.\n");
        return -1;
    }

    ObjectEvent *event = (ObjectEvent *)out;
    event->eventSubtype = 0;
    event->actionCount = 0;
    event->actions = NULL;

    return 0;
}


static int PhysicsVertex_parse(Reader *reader, PhysicsVertex *vertex) {
    if (reader == NULL || vertex == NULL) {
        logWarn("[PhysicsVertex_parse] Invalid parameters: reader=%p, vertex=%p\n", (void*)reader, (void*)vertex);
        return -1;
    }

    read(&vertex->x, Float32);
    read(&vertex->y, Float32);

    return 0;
}

static int GameObject_free(GameObject *obj) {
    if (obj == NULL) return -1;

    obj->name = NULL;

    if (obj->physicsVertices != NULL) {
        free(obj->physicsVertices);
        obj->physicsVertices = NULL;
    }

    for (uint32_t eventType = 0; eventType < OBJT_EVENT_TYPE_COUNT; eventType++) {
        ObjectEventList *eventList = &obj->eventLists[eventType];
        if (eventList->events != NULL) {
            for (uint32_t eventIndex = 0; eventIndex < eventList->eventCount; eventIndex++) {
                ObjectEvent *event = &eventList->events[eventIndex];
                if (event->actions != NULL) {
                    free(event->actions);
                    event->actions = NULL;
                }
            }
            free(eventList->events);
            eventList->events = NULL;
        }
        eventList->eventCount = 0;
    }

    return 0;
}

int OBJT_free(ObjtChunk *objt) {
    if (objt == NULL) return -1;

    if (objt->objects != NULL) {
        for (uint32_t i = 0; i < objt->count; i++) {
            GameObject_free(&objt->objects[i]);
        }
        free(objt->objects);
        objt->objects = NULL;
    }

    objt->count = 0;

    return 0;
}