#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <err.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <arpa/inet.h>

#include "reactor.h"
#include "reactor_signal.h"
#include "reactor_signal_dispatcher.h"
#include "reactor_resolver.h"

reactor_resolver *reactor_resolver_new(reactor *r, reactor_handler *h, char *name, char *service, void *user)
{
  reactor_resolver *s;
  int e;
  
  s = malloc(sizeof *s);
  e = reactor_resolver_construct(r, s, h, name, service, user);
  if (e == -1)
    {
      reactor_resolver_delete(s);
      return NULL;
    }

  return s;
}

int reactor_resolver_construct(reactor *r, reactor_resolver *s, reactor_handler *h, char *name, char *service, void *data)
{
  struct sigevent sigev;
  struct gaicb *ar_list[] = {&s->ar};
  int e;
  
  *s = (reactor_resolver) {.user = {.handler = h, .data = data}, .reactor = r,
			   .signal = {.handler = reactor_resolver_handler, .data = s},
			   .ar = {.ar_name = name, .ar_service = service}};
  s->dispatcher = reactor_signal_dispatcher_new(r);
  if (!s->dispatcher)
    return -1;

  sigev = (struct sigevent) {.sigev_notify = SIGEV_SIGNAL, .sigev_signo = REACTOR_SIGNAL_DISPATCHER_SIGNO,
			     .sigev_value.sival_ptr = &s->signal};
  e = getaddrinfo_a(GAI_NOWAIT, ar_list, 1, &sigev);
  if (e == -1)
    return -1;

  return 0;
}

int reactor_resolver_destruct(reactor_resolver *s)
{
  int e;
  
  if (s->dispatcher)
    {
      e = gai_cancel(&s->ar);
      if (e != EAI_ALLDONE)
	return -1;

      if (s->ar.ar_result)
	freeaddrinfo(s->ar.ar_result);

      e = reactor_signal_dispatcher_delete(s->dispatcher);
      if (e == -1)
	return -1;
      s->dispatcher = NULL;
    }

  return 0;
}

int reactor_resolver_delete(reactor_resolver *s)
{
  int e;
  
  e = reactor_resolver_destruct(s);
  if (e == -1)
    return -1;

  free(s);
  return 0;
}

void reactor_resolver_handler(reactor_event *e)
{
  reactor_resolver *s = e->call->data;

  reactor_dispatch(&s->user, REACTOR_RESOLVER_RESULT, s);
  //  freeaddrinfo(s->ar.ar_result);
}
