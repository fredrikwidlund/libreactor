#ifndef REACTOR_UTIL_H_INCLUDED
#define REACTOR_UTIL_H_INCLUDED

#define reactor_likely(x)   __builtin_expect(!!(x),1)
#define reactor_unlikely(x) __builtin_expect(!!(x),0)

size_t reactor_util_u32len(uint32_t);
void   reactor_util_u32sprint(uint32_t, char *);
void   reactor_util_u32toa(uint32_t, char *);

#endif /* REACTOR_UTIL_H_INCLUDED */
