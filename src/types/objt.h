#ifndef OBJT_TYPES_H
#define OBJT_TYPES_H

#include <stdint.h>
#include "tmln.h"

#define OBJT_EVENT_TYPE_COUNT 15

typedef struct {
    uint32_t eventSubtype;
    uint32_t actionCount;
    EventAction* actions;
} ObjectEvent;

typedef struct {
    uint32_t eventCount;
    ObjectEvent* events;
} ObjectEventList;

typedef struct {
    float x;
    float y;
} PhysicsVertex;

typedef struct {
    bool present;
    const char* name;
    int32_t spriteId;
    bool visible;
    bool managed; // GMS 2022.5+
    bool solid;
    int32_t depth;
    bool persistent;
    int32_t parentId;
    int32_t textureMaskId;
    bool usesPhysics;
    bool isSensor;
    uint32_t collisionShape;
    float density;
    float restitution;
    uint32_t group;
    float linearDamping;
    float angularDamping;
    int32_t physicsVertexCount;
    float friction;
    bool awake;
    bool kinematic;
    PhysicsVertex* physicsVertices;
    ObjectEventList eventLists[OBJT_EVENT_TYPE_COUNT];
} GameObject;

typedef struct {
    uint32_t count;
    GameObject* objects;
} ObjtChunk;

#endif