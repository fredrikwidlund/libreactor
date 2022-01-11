#include <string.h>
#include <netdb.h>

#include "reactor.h"

static void resolver_callback(reactor_event *event)
{
  resolver *resolver = event->state;

  switch (event->type)
  {
  case ASYNC_CALL:
    resolver->addrinfo = net_resolve(resolver->host, resolver->service, resolver->family, resolver->type, resolver->flags);
    break;
  default:
  case ASYNC_DONE:
    reactor_dispatch(&resolver->handler,
                     resolver->addrinfo ? RESOLVER_SUCCESS : RESOLVER_FAILURE,
                     (uintptr_t) resolver->addrinfo);
    break;
  }
}

void resolver_construct(resolver *resolver, reactor_callback *callback, void *state)
{
  *resolver = (struct resolver) {0};
  reactor_handler_construct(&resolver->handler, callback, state);
  async_construct(&resolver->async, resolver_callback, resolver);
}

void resolver_destruct(resolver *resolver)
{
  async_destruct(&resolver->async);
  resolver_clear(resolver);
}

void resolver_lookup(resolver *resolver, char *host, char *service, int family, int type, int protocol, int flags)
{
  resolver_cancel(resolver);
  if (host)
    resolver->host = strdup(host);
  if (service)
    resolver->service = strdup(service);
  resolver->family = family;
  resolver->type = type;
  resolver->protocol = protocol;
  resolver->flags = flags;
  async_call(&resolver->async);
}

void resolver_cancel(resolver *resolver)
{
  async_cancel(&resolver->async);
  resolver_clear(resolver);
}

void resolver_clear(resolver *resolver)
{
  if (resolver->addrinfo)
    freeaddrinfo(resolver->addrinfo);
  free(resolver->host);
  free(resolver->service);
  resolver->addrinfo = NULL;
  resolver->host = NULL;
  resolver->service = NULL;
}

