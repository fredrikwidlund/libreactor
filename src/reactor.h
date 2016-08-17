#ifndef REACTOR_H_INCLUDED
#define REACTOR_H_INCLUDED

#define REACTOR_VERSION "0.9.0"
#define REACTOR_VERSION_MAJOR 0
#define REACTOR_VERSION_MINOR 9
#define REACTOR_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

#include "reactor/reactor_user.h"
#include "reactor/reactor_desc.h"
#include "reactor/reactor_core.h"
#include "reactor/reactor_event.h"
#include "reactor/reactor_timer.h"
#include "reactor/reactor_stream.h"
#include "reactor/reactor_tcp.h"
#include "reactor/reactor_http.h"
#include "reactor/reactor_rest.h"

#ifdef __cplusplus
}
#endif

#endif /* REACTOR_H_INCLUDED */
