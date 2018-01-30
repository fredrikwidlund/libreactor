#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include <dynamic.h>

#include "reactor_util.h"
#include "reactor_user.h"
#include "reactor_core.h"
#include "reactor_descriptor.h"
#include "reactor_pool.h"
#include "reactor_descriptor.h"
#include "reactor_resolver.h"

#ifndef GCOV_BUILD
static
#endif
int reactor_resolver_event(void *state, int type, void *data)
{
  reactor_resolver *resolver = state;

  (void) data;
  switch (type)
    {
    case REACTOR_POOL_EVENT_CALL:
      (void) getaddrinfo(resolver->node, resolver->service, &resolver->hints, &resolver->addrinfo);
      return REACTOR_OK;
    case REACTOR_POOL_EVENT_RETURN:
      return reactor_user_dispatch(&resolver->user, REACTOR_RESOLVER_EVENT_DONE, resolver->addrinfo);
    default:
      return reactor_user_dispatch(&resolver->user, REACTOR_RESOLVER_EVENT_ERROR, NULL);
    }
}

int reactor_resolver_open(reactor_resolver *resolver, reactor_user_callback *callback, void *state,
                          char *node, char *service, int family, int socktype, int flags)
{
  reactor_user_construct(&resolver->user, callback, state);
  resolver->node = strdup(node);
  resolver->service = strdup(service);
  resolver->hints = (struct addrinfo) {.ai_family = family, .ai_socktype = socktype, .ai_flags = flags};
  reactor_core_enqueue(reactor_resolver_event, resolver);
  return REACTOR_OK;
}

void reactor_resolver_close(reactor_resolver *resolver)
{
  freeaddrinfo(resolver->addrinfo);
  free(resolver->node);
  free(resolver->service);
  *resolver = (reactor_resolver) {0};
}
