#ifndef UTILS_H
#define UTILS_H

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

#endif