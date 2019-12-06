#ifndef REACTOR_H_INCLUDED
#define REACTOR_H_INCLUDED

#define REACTOR_VERSION       "1.0.1"
#define REACTOR_VERSION_MAJOR 1
#define REACTOR_VERSION_MINOR 0
#define REACTOR_VERSION_PATCH 1

#ifdef __cplusplus
extern "C" {
#endif

#include "reactor/reactor.h"
#include "reactor/reactor_assert.h"
#include "reactor/reactor_vector.h"
#include "reactor/reactor_user.h"
#include "reactor/reactor_utility.h"
#include "reactor/reactor_core.h"
#include "reactor/reactor_stats.h"
#include "reactor/reactor_fd.h"
#include "reactor/reactor_pool.h"
#include "reactor/reactor_timer.h"
#include "reactor/reactor_resolver.h"
#include "reactor/reactor_net.h"
#include "reactor/reactor_stream.h"
#include "reactor/reactor_http.h"
#include "reactor/reactor_couch.h"
#include "reactor/reactor_server.h"

#ifdef __cplusplus
}
#endif

#endif /* REACTOR_H_INCLUDED */

