#ifndef MAIN_REACTOR_CORE_H_INCLUDED
#define MAIN_REACTOR_CORE_H_INCLUDED

#define REACTOR_CORE_VERSION "0.3.0"
#define REACTOR_CORE_VERSION_MAJOR 0
#define REACTOR_CORE_VERSION_MINOR 3
#define REACTOR_CORE_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

#include "reactor_core/reactor_user.h"
#include "reactor_core/reactor_desc.h"
#include "reactor_core/reactor_core.h"
#include "reactor_core/reactor_timer.h"
#include "reactor_core/reactor_stream.h"
#include "reactor_core/reactor_signal.h"
#include "reactor_core/reactor_signal_dispatcher.h"

#ifdef __cplusplus
}
#endif

#endif /* MAIN_REACTOR_CORE_H_INCLUDED */
