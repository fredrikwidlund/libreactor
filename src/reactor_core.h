#ifndef MAIN_REACTOR_CORE_H_INCLUDED
#define MAIN_REACTOR_CORE_H_INCLUDED

#define REACTOR_CORE_VERSION "1.0.0"
#define REACTOR_CORE_VERSION_MAJOR 1
#define REACTOR_CORE_VERSION_MINOR 0
#define REACTOR_CORE_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

#include "reactor_core/reactor_user.h"
#include "reactor_core/reactor_desc.h"
#include "reactor_core/reactor_core.h"
#include "reactor_core/reactor_event.h"
#include "reactor_core/reactor_timer.h"
#include "reactor_core/reactor_resolver.h"
#include "reactor_core/reactor_stream.h"
#include "reactor_core/reactor_tcp.h"
#include "reactor_core/reactor_http.h"
#include "reactor_core/reactor_rest.h"

#ifdef __cplusplus
}
#endif

#endif /* MAIN_REACTOR_CORE_H_INCLUDED */
