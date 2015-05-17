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
#include "reactor_resolver.h"

reactor_resolver *reactor_resolver_new(reactor *r, reactor_handler *h, void *user)
{
  reactor_resolver *s;
  int e;
  
  s = malloc(sizeof *s);
  e = reactor_resolver_construct(r, s, h, user);
  if (e == -1)
    {
      reactor_resolver_delete(s);
      return NULL;
    }

  return s;
}

int reactor_resolver_construct(reactor *r, reactor_resolver *s, reactor_handler *h, void *data)
{
  int e;
  
  *s = (reactor_resolver) {.user = {.handler = h, .data = data}, .reactor = r};
  e = reactor_signal_construct(r, &s->signal, reactor_resolver_handler, SIGUSR1, s);
  if (e == -1)
    return -1;
  
  return 0;
}

int reactor_resolver_destruct(reactor_resolver *s)
{
  return reactor_signal_destruct(&s->signal);
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
  struct signalfd_siginfo *fdsi = e->data;
  struct gaicb *ar = (struct gaicb *) fdsi->ssi_ptr;
  struct addrinfo *ai;
  struct sockaddr_in *sin;
  char name[4096];
  //reactor_resolver *s = e->call->data;

  (void) fprintf(stderr, "[signal SIGUSR1 raised, pid %d, int %d, ptr %p\n", fdsi->ssi_pid, fdsi->ssi_int, (void *) fdsi->ssi_ptr);
  
  for (ai = ar->ar_result; ai; ai = ai->ai_next)
    {
      if (ai->ai_family == AF_INET)
	{
	  sin = (struct sockaddr_in *) ai->ai_addr;
	  (void) fprintf(stderr, "result: %s -> %s\n", ar->ar_name, inet_ntop(AF_INET, &sin->sin_addr, name, sizeof name));
	}
    }
  
  freeaddrinfo(ar->ar_result);
  free(ar);
}

int reactor_resolver_lookup(reactor_resolver *s, char *node, char *service)
{
  struct gaicb *ar;
  int e;
  
  ar = malloc(sizeof *ar);
  *ar = (struct gaicb) {.ar_name = node, .ar_service = service};
  e = getaddrinfo_a(GAI_NOWAIT, &ar, 1, (struct sigevent[]) {{.sigev_notify = SIGEV_SIGNAL, .sigev_signo = SIGUSR1, .sigev_value.sival_ptr = ar}});
  if (e == -1)
    return -1;

  return 0;
}
