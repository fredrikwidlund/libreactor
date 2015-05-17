#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <err.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <arpa/inet.h>

#include "reactor.h"
#include "reactor_signal.h"
#include "reactor_signal_dispatcher.h"
#include "reactor_resolver.h"

void handler(reactor_event *e)
{
  reactor *r = e->call->data;
  reactor_resolver *s = e->data;
  struct addrinfo *ai = s->list[0]->ar_result;
  char name[256];

  if (ai)
    {
      inet_ntop(AF_INET, &((struct sockaddr_in *) ai->ai_addr)->sin_addr, name, sizeof name);
      (void) fprintf(stderr, "-> %s\n", name);
    }
  
  reactor_halt(r);
}

int main()
{
  reactor *r = reactor_new(); 
  reactor_resolver *s = reactor_resolver_new_simple(r, handler, "www.svtplay.se", "http", r);
  reactor_run(r);
  reactor_resolver_delete(s);
  reactor_delete(r);
}
