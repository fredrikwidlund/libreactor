#ifndef REACTOR_UTIL_H_INCLUDED
#define REACTOR_UTIL_H_INCLUDED

#define reactor_likely(x)   __builtin_expect(!!(x),1)
#define reactor_unlikely(x) __builtin_expect(!!(x),0)

#endif /* REACTOR_UTIL_H_INCLUDED */
