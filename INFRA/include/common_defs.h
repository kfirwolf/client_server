// common_defs.h
#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#ifndef likely
#define likely(x)   __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#endif // COMMON_DEFS_H