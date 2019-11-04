#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <dynamic.h>

#include "reactor.h"

reactor_status resolver_handler(reactor_event *event)
{
  struct addrinfo *ai = (struct addrinfo *) event->data;
  char host[NI_MAXHOST], service[NI_MAXSERV];
  int e;

  e = getnameinfo(ai->ai_addr, ai->ai_addrlen,
                  host, NI_MAXHOST,
                  service, NI_MAXSERV,
                  NI_NUMERICHOST | NI_NUMERICSERV);
  if (e != 0)
    err(1, "getnameinfo");

  printf("%s:%s\n", host, service);

  return REACTOR_OK;
}

int main()
{
  (void) reactor_construct();
  (void) reactor_resolver_request(resolver_handler, NULL, "www.google.com", "https", AF_INET, SOCK_STREAM, 0);
  (void) reactor_run();

  reactor_destruct();
}
