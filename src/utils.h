#ifndef UTILS_H
#define UTILS_H

#include "log.h"
#include <stdarg.h>
#include <stdlib.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L)
    #define TYPEOF(x) typeof(x)
#elif defined(_MSC_VER) && defined(__cplusplus) && __cplusplus >= 201103L
    #define TYPEOF(x) std::remove_reference<decltype(x)>::type
#elif defined(__GNUC__) || defined(__clang__) || \
    (defined(__TINYC__) && __TINYC__ >= 913) || \
    (defined(_MSC_VER) && _MSC_VER >= 1940 && !defined(__cplusplus))
    #define TYPEOF(x) __typeof__(x)
#else
    #define TYPEOF(x) int64_t
#endif

#define forEach(type, item, array, count) \
    for (TYPEOF(count) item##_i_ = 0; item##_i_ < (count); ++item##_i_) \
    for (type* item = &(array)[item##_i_]; item; item = NULL)

#define forEachIndexed(type, item, index, array, count) \
    for (TYPEOF(count) index = 0; index < (count); ++index) \
    for (type* item = &(array)[index]; item; item = NULL)

#define repeat(n, it) for (TYPEOF(n) it = 0; it < (n); ++it)

#define require(condition) \
    do { \
        if (!(condition)) { \
        logError("Requirement failed at %s:%d\n", __FILE__, __LINE__); \
        abort(); \
    } \
} while (0)

#define requireMessage(condition, message) \
    do { \
        if (!(condition)) { \
        logError("Requirement failed at %s:%d: %s\n", __FILE__, __LINE__, message); \
        abort(); \
	} \
} while (0)

static inline void requireMessageFormatted(const char *file, int line, bool condition, const char *fmt, ...) {
    if (condition)
        return;
    va_list args;
    logError("Requirement failed at %s:%d: ", file, line);
    va_start(args, fmt);
    vLogError(fmt, args);
    va_end(args);
    logError("\n");
    abort();
}

static inline void* requireNotNullFunction(void* ptr, const char* file, int line, const char* name) {
    if (!ptr) {
        logError("%s:%d: requireNotNull failed: '%s'\n", file, line, name);
        abort();
    }
    return ptr;
}
#define requireNotNull(ptr) requireNotNullFunction((void*)ptr, __FILE__, __LINE__, #ptr)
#define requireNotNullMessage(ptr, msg) requireNotNullFunction((void*)ptr, __FILE__, __LINE__, msg)

// Safe allocation macros - check for nullptr and abort with file/line info
static inline void *safeMallocFunction(size_t size, const char *file, int line) {
    if (size == 0)
        return NULL;
    void *ret = malloc(size);
    if (!ret) {
        logError("FATAL: malloc(%zu) failed at %s:%d\n", size, file, line);
        abort();
    }
    return ret;
}
#define safeMalloc(size) safeMallocFunction(size, __FILE__, __LINE__)

static inline void *safeCallocFunction(size_t count, size_t size, const char *file, int line) {
    if (size == 0 || count == 0)
        return NULL;
    void *ret = calloc(count, size);
    if (!ret) {
        logError("FATAL: calloc(%zu, %zu) failed at %s:%d\n", count, size, file, line);
        abort();
    }
    return ret;
}
#define safeCalloc(count, size) safeCallocFunction(count, size, __FILE__, __LINE__)

static inline void *safeReallocFunction(void *ptr, size_t size, const char *file, int line) {
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    void *ret = realloc(ptr, size);
    if (!ret) {
        logError("FATAL: realloc(%zu) failed at %s:%d\n", size, file, line);
        abort();
    }
    return ret;
}
#define safeRealloc(ptr, size) safeReallocFunction(ptr, size, __FILE__, __LINE__)

#endif