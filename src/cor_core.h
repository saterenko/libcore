#ifndef COR_CORE_H
#define COR_CORE_H

#define cor_ok 0
#define cor_error -1

#define cor_hash(key, c) ((unsigned int) key * 31 + c)

#if defined(__GNUC__) || defined(__clang__)
#define cor_likely(x) __builtin_expect(!!(x), 1)
#else
#define cor_likely(x) (x)
#endif

#if defined(__GNUC__) || defined(__clang__)
#define cor_unlikely(x) __builtin_expect(!!(x), 0)
#else
#define cor_unlikely(x) (x)
#endif

#endif
