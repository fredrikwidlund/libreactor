#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sched.h>
#include <err.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_event.h"
#include "reactor_resolver.h"

static inline void reactor_resolver_close_final(reactor_resolver *resolver)
{
  if (resolver->state == REACTOR_RESOLVER_CLOSE_WAIT &&
      resolver->event.desc.state == REACTOR_DESC_CLOSED &&
      resolver->ref == 0)
    {
      resolver->state = REACTOR_RESOLVER_CLOSED;
      reactor_user_dispatch(&resolver->user, REACTOR_RESOLVER_CLOSE, NULL);
    }
}

static inline void reactor_resolver_hold(reactor_resolver *resolver)
{
  resolver->ref ++;
}

static inline void reactor_resolver_release(reactor_resolver *resolver)
{
  resolver->ref --;
  reactor_resolver_close_final(resolver);
}

static int reactor_resolver_clone(void *arg)
{
  reactor_resolver *resolver = arg;
  ssize_t n;

  return 0;
}

static void reactor_resolver_lookup(reactor_resolver *resolver)
{
  int e;

  if (fork() == 0)
    {
      write(reactor_desc_fd(&resolver->event.desc), (uint64_t[]){{1}}, sizeof(uint64_t));
      exit(0);
    }
  /*
  //e = clone(reactor_resolver_clone, top, CLONE_FILES | CLONE_FS | CLONE_IO | CLONE_THREAD | CLONE_VM | SIGCHLD, resolver);
  e = clone(reactor_resolver_clone, resolver->stack + 1048576, CLONE_FILES | CLONE_VM, resolver);
  if (e == -1)
    reactor_resolver_error(resolver);
  */
}

void reactor_resolver_init(reactor_resolver *resolver, reactor_user_callback *callback, void *state)
{
  *resolver = (reactor_resolver) {.state = REACTOR_RESOLVER_CLOSED};
  reactor_user_init(&resolver->user, callback, state);
  reactor_event_init(&resolver->event, reactor_resolver_event, resolver);
}

void reactor_resolver_open(reactor_resolver *resolver, char *node, char *service, struct addrinfo *hints)
{
  if (resolver->state != REACTOR_RESOLVER_CLOSED)
    {
      reactor_resolver_error(resolver);
      return;
    }
  resolver->node = node;
  resolver->service = service;
  resolver->hints = hints;
  resolver->state = REACTOR_RESOLVER_OPEN;

  reactor_resolver_hold(resolver);
  reactor_event_open(&resolver->event);
  if (resolver->state == REACTOR_RESOLVER_OPEN)
    reactor_resolver_lookup(resolver);
  reactor_resolver_release(resolver);
}

void reactor_resolver_error(reactor_resolver *resolver)
{
  resolver->state = REACTOR_RESOLVER_INVALID;
  reactor_user_dispatch(&resolver->user, REACTOR_RESOLVER_ERROR, NULL);
}

void reactor_resolver_close(reactor_resolver *resolver)
{
  if (resolver->state == REACTOR_RESOLVER_CLOSED)
    return;
  resolver->state = REACTOR_RESOLVER_CLOSE_WAIT;
  reactor_event_close(&resolver->event);
  reactor_resolver_close_final(resolver);
}

void reactor_resolver_event(void *state, int type, void *data)
{
  reactor_resolver *resolver = state;
  uint64_t value;
  ssize_t n;

  (void) data;
  switch(type)
    {
    case REACTOR_EVENT_SIGNAL:
      reactor_event_close(&resolver->event);
      break;
    case REACTOR_EVENT_ERROR:
      err(1, "error");
      reactor_resolver_error(resolver);
      break;
    case REACTOR_EVENT_SHUTDOWN:
      err(1, "shut");
      break;
    case REACTOR_EVENT_CLOSE:
      reactor_resolver_close_final(resolver);
      break;
    }
}
