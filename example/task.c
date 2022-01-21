#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <assert.h>

#include <reactor.h>

/* async */

static void *async_thread(void *arg)
{
  async *async = arg;

  reactor_dispatch(&async->handler, ASYNC_CALL, 0);
  event_signal(&async->event);
  return NULL;
}

static void async_cancel(async *async)
{
  if (event_is_open(&async->event))
  {
    event_close(&async->event);
    if (async->thread)
    {
      (void) pthread_cancel(async->thread);
      async->thread = 0;
    }
  }
}

static void async_callback(reactor_event *event)
{
  async *async = event->state;

  assert(event->type == EVENT_SIGNAL);
  async->thread = 0;
  async_close(async);
  reactor_dispatch(&async->handler, ASYNC_DONE, 0);
}

static void async_construct(async *async, reactor_callback *callback, void *state)
{
  *async = (struct async) {0};
  reactor_handler_construct(&async->handler, callback, state);
  event_construct(&async->event, async_callback, async);
}

static void async_destruct(async *async)
{
  async_close(async);
  event_destruct(&async->event);
}

static void async_call(async *async)
{
  int e;
  pthread_attr_t attr;

  event_open(&async->event);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  e = pthread_create(&async->thread, &attr, async_thread, async);
  assert(e == 0);
}

/* resolver */

enum
{
  RESOLVER_SUCCESS,
  RESOLVER_FAILURE
};

typedef struct resolver resolver;
struct resolver
{
  reactor_handler handler;
  async async;
  char *host;
  char *service;
  int family;
  int type;
  int protocol;
  int flags;
  struct addrinfo *addrinfo;
};

static void resolver_callback(reactor_event *event)
{
  resolver *resolver = event->state;

  switch (event->type)
  {
  case ASYNC_CALL:
    resolver->addrinfo = net_resolve(resolver->host, resolver->service, resolver->family, resolver->type, resolver->flags);
    break;
  case ASYNC_DONE:
    reactor_dispatch(&resolver->handler,
                     resolver->addrinfo ? RESOLVER_SUCCESS : RESOLVER_FAILURE,
                     (uintptr_t) resolver->addrinfo);
    break;
  }
}

static void resolver_construct(resolver *resolver, reactor_callback *callback, void *state)
{
  *resolver = (struct resolver) {0};
  reactor_handler_construct(&resolver->handler, callback, state);
  async_construct(&resolver->async, resolver_callback, resolver);
}

static void resolver_clear(resolver *resolver)
{
  if (resolver->addrinfo)
    freeaddrinfo(resolver->addrinfo);
  free(resolver->host);
  free(resolver->service);
  resolver->addrinfo = NULL;
  resolver->host = NULL;
  resolver->service = NULL;
}

static void resolver_cancel(resolver *resolver)
{
  async_close(&resolver->async);
  resolver_clear(resolver);
}

static void resolver_destruct(resolver *resolver)
{
  async_destruct(&resolver->async);
  resolver_clear(resolver);
}

static void resolver_lookup(resolver *resolver, char *host, char *service, int family, int type, int protocol, int flags)
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

static void callback(reactor_event *event)
{
  resolver *resolver = event->state;
  struct addrinfo *addrinfo;
  char host[NI_MAXHOST], service[NI_MAXSERV];
  int e;

  switch (event->type)
  {
  case RESOLVER_SUCCESS:
    addrinfo = (struct addrinfo *) event->data;
    do
    {
      e = getnameinfo(addrinfo->ai_addr, addrinfo->ai_addrlen, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
      assert(e == 0);
      (void) printf("%s resolved to %s\n", resolver->host, host);
      addrinfo = addrinfo->ai_next;
    }
    while (addrinfo);
    break;
  case RESOLVER_FAILURE:
    (void) printf("%s did not resolve\n", resolver->host);
    break;
  }
}

int main(int argc, char **argv)
{
  resolver r[argc - 1];
  int i;

  reactor_construct();
  for (i = 0; i < argc - 1; i++)
  {
    resolver_construct(&r[i], callback, &r[i]);
    resolver_lookup(&r[i], argv[i + 1], NULL, 0, SOCK_STREAM, 0, 0);
  }
  reactor_loop();
  for (i = 0; i < argc - 1; i++)
    resolver_destruct(&r[i]);
  reactor_destruct();
}
