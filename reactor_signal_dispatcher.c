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

static _Thread_local reactor_signal_dispatcher *reactor_signal_dispatcher_singleton = NULL;

reactor_signal_dispatcher *reactor_signal_dispatcher_new(reactor *r)
{
  reactor_signal_dispatcher *s = reactor_signal_dispatcher_singleton;
  int e;

  if (s != NULL)
    {
      s->ref ++;
      return s;
    }
  
  s = malloc(sizeof *s);
  e = reactor_signal_dispatcher_construct(r, s);
  if (e == -1)
    {
      reactor_signal_dispatcher_delete(s);
      return NULL;
    }

  reactor_signal_dispatcher_singleton = s;
  return s;
}

int reactor_signal_dispatcher_construct(reactor *r, reactor_signal_dispatcher *s)
{
  int e;
  
  *s = (reactor_signal_dispatcher) {.reactor = r, .ref = 1};
  e = reactor_signal_construct(r, &s->signal, reactor_signal_dispatcher_handler, REACTOR_SIGNAL_DISPATCHER_SIGNO, s);
  if (e == -1)
    return -1;
  
  return 0;
}

int reactor_signal_dispatcher_destruct(reactor_signal_dispatcher *s)
{
  return reactor_signal_destruct(&s->signal);
}

int reactor_signal_dispatcher_delete(reactor_signal_dispatcher *s)
{
  int e;

  s->ref --;
  if (!s->ref)
    {
      e = reactor_signal_dispatcher_destruct(s);
      if (e == -1)
	return -1;
      
      free(s);
      reactor_signal_dispatcher_singleton = NULL;
    }
  
  return 0;
}

void reactor_signal_dispatcher_handler(reactor_event *e)
{
  struct signalfd_siginfo *fdsi = e->data;

  if (fdsi->ssi_pid == getpid() && fdsi->ssi_uid == getuid())
    reactor_dispatch((reactor_call *) fdsi->ssi_ptr, REACTOR_SIGNAL_DISPATCHER_RAISED, fdsi); 
}
