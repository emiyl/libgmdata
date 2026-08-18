#ifndef PATH_TYPES_H
#define PATH_TYPES_H

#include <stdint.h>

typedef struct {
    float x;
    float y;
    float speed;
} PathPoint;

typedef struct {
    float x;
    float y;
    float speed;
    float l; // cumulative arc length from start
} InternalPathPoint;

typedef struct {
    float x;
    float y;
    float speed;
} PathPositionResult;

typedef struct {
    bool present;
    const char* name;
    bool isSmooth;
    bool isClosed;
    uint32_t precision;
    uint32_t pointCount;
    PathPoint* points;
    uint32_t internalPointCount;
    InternalPathPoint* internalPoints;
    float length; // total arc length
    bool exists;
} GamePath;

typedef struct {
    uint32_t count;
    GamePath* paths;
} Path;

#endif