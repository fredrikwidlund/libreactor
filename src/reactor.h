#ifndef REACTOR_H_INCLUDED
#define REACTOR_H_INCLUDED

#define REACTOR_VERSION       "2.0.0-alpha"
#define REACTOR_VERSION_MAJOR 2
#define REACTOR_VERSION_MINOR 0
#define REACTOR_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <dynamic.h>

#include "reactor/reactor.h"
#include "reactor/timer.h"
#include "reactor/notify.h"
#include "reactor/stream.h"
#include "reactor/http.h"
#include "reactor/server.h"
#include "reactor/net.h"

#ifdef __cplusplus
}
#endif

#endif /* REACTOR_H_INCLUDED */
