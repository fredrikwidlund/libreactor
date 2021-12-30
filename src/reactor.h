#ifndef REACTOR_H_INCLUDED
#define REACTOR_H_INCLUDED

#define REACTOR_VERSION       "3.0.0"
#define REACTOR_VERSION_MAJOR 3
#define REACTOR_VERSION_MINOR 0
#define REACTOR_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include "reactor/data.h"
#include "reactor/buffer.h"
#include "reactor/vector.h"
#include "reactor/list.h"
#include "reactor/hash.h"
#include "reactor/map.h"
#include "reactor/mapi.h"
#include "reactor/maps.h"
#include "reactor/utility.h"
#include "reactor/string.h"

#include "reactor/reactor.h"
#include "reactor/descriptor.h"
#include "reactor/stream.h"
#include "reactor/net.h"
#include "reactor/http.h"
#include "reactor/server.h"
#include "reactor/timer.h"
#include "reactor/notify.h"

#ifdef __cplusplus
}
#endif

#endif /* REACTOR_H_INCLUDED */
