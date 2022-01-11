#ifndef REACTOR_H_INCLUDED
#define REACTOR_H_INCLUDED

#define REACTOR_VERSION       "3.0.0"
#define REACTOR_VERSION_MAJOR 3
#define REACTOR_VERSION_MINOR 0
#define REACTOR_VERSION_PATCH 0

#ifdef UNIT_TESTING
extern void mock_assert(const int, const char * const, const char * const, const int);
#undef assert
#define assert(expression) mock_assert((int)(expression), #expression, __FILE__, __LINE__);
#endif /* UNIT_TESTING */

#define reactor_likely(x)   __builtin_expect(!!(x), 1)
#define reactor_unlikely(x) __builtin_expect(!!(x), 0)

#ifdef __cplusplus
extern "C" {
#endif

#include "reactor/data.h"
#include "reactor/pointer.h"
#include "reactor/list.h"
#include "reactor/buffer.h"
#include "reactor/vector.h"
#include "reactor/hash.h"
#include "reactor/map.h"
#include "reactor/mapi.h"
#include "reactor/maps.h"
#include "reactor/utility.h"
#include "reactor/string.h"
#include "reactor/core.h"
#include "reactor/descriptor.h"
#include "reactor/event.h"
#include "reactor/stream.h"
#include "reactor/net.h"
#include "reactor/http.h"
#include "reactor/timer.h"
#include "reactor/notify.h"
#include "reactor/server.h"
#include "reactor/async.h"
#include "reactor/resolver.h"

#ifdef __cplusplus
}
#endif

#endif /* REACTOR_H_INCLUDED */
