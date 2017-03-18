#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_pool.h"
#include "reactor_core.h"
#include "reactor_resolver.h"

static void reactor_resolver_error(reactor_resolver *resolver)
{
  reactor_resolver_hold(resolver);
  resolver->state = REACTOR_RESOLVER_STATE_ERROR;
  reactor_user_dispatch(&resolver->user, REACTOR_RESOLVER_EVENT_ERROR, NULL);
  reactor_resolver_close(resolver);
  reactor_resolver_release(resolver);
}

static void reactor_resolver_job(void *state, int type, void *data)
{
  reactor_resolver *resolver = state;

  (void) data;
  switch (type)
    {
    case REACTOR_POOL_EVENT_CALL:
      (void) getaddrinfo(resolver->node, resolver->service, &resolver->hints, &resolver->addrinfo);
      break;
    case REACTOR_POOL_EVENT_RETURN:
      reactor_resolver_hold(resolver);
      reactor_user_dispatch(&resolver->user, REACTOR_RESOLVER_EVENT_RESULT, resolver->addrinfo);
      freeaddrinfo(resolver->addrinfo);
      reactor_resolver_close(resolver);
      reactor_resolver_release(resolver);
      break;
    }
}

static void reactor_resolver_resolve(reactor_resolver *resolver, char *node, char *service)
{
  int e, flags_saved = resolver->hints.ai_flags;
  struct addrinfo *addrinfo = NULL;

  resolver->hints.ai_flags |= AI_NUMERICHOST | AI_NUMERICSERV;
  e = getaddrinfo(node, service, &resolver->hints, &addrinfo);
  if (e == -1)
    {
      reactor_resolver_error(resolver);
      return;
    }

  if (addrinfo)
    {
      reactor_resolver_hold(resolver);
      reactor_user_dispatch(&resolver->user, REACTOR_RESOLVER_EVENT_RESULT, addrinfo);
      freeaddrinfo(addrinfo);
      reactor_resolver_close(resolver);
      reactor_resolver_release(resolver);
      return;
    }

  resolver->hints.ai_flags = flags_saved;
  resolver->node = strdup(node);
  resolver->service = strdup(service);
  reactor_core_job_register(reactor_resolver_job, resolver);
}

void reactor_resolver_hold(reactor_resolver *resolver)
{
  resolver->ref ++;
}

void reactor_resolver_release(reactor_resolver *resolver)
{
  resolver->ref --;
  if (!resolver->ref)
    {
      resolver->state = REACTOR_RESOLVER_STATE_CLOSED;
      reactor_user_dispatch(&resolver->user, REACTOR_RESOLVER_EVENT_CLOSE, NULL);
    }
}

void reactor_resolver_open(reactor_resolver *resolver, reactor_user_callback *callback, void *state,
                           char *node, char *service, struct addrinfo *hints)
{
  resolver->ref = 0;
  resolver->state = REACTOR_RESOLVER_STATE_OPEN;
  resolver->node = NULL;
  resolver->service = NULL;
  resolver->addrinfo = NULL;
  reactor_user_construct(&resolver->user, callback, state);
  resolver->hints = hints ? *hints : (struct addrinfo){.ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
  reactor_resolver_hold(resolver);
  reactor_resolver_resolve(resolver, node, service);
}

void reactor_resolver_close(reactor_resolver *resolver)
{
  if (resolver->state & REACTOR_RESOLVER_STATE_CLOSED)
    return;

  resolver->state = REACTOR_RESOLVER_STATE_CLOSED;
  reactor_resolver_release(resolver);
}
