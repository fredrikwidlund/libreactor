#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

#include <dynamic.h>

#include "reactor.h"

struct tcpd
{
  reactor_net net;
};

reactor_status net_handler(reactor_event *event)
{
  struct tcpd *tcpd = event->state;
  ssize_t n __attribute__((unused));
  int fd;

  switch (event->type)
    {
    case REACTOR_NET_EVENT_ERROR:
      (void) fprintf(stderr, "%s\n", (char *) event->data);
      reactor_net_destruct(&tcpd->net);
      return REACTOR_ABORT;
    case REACTOR_NET_EVENT_ACCEPT:
      fd = event->data;
      n = write(fd, "hello\n", 6);
      (void) close(fd);
      reactor_net_destruct(&tcpd->net);
      return REACTOR_ABORT;
    }

  return REACTOR_OK;
}

int main()
{
  struct tcpd tcpd;
  int e;

  (void) reactor_construct();
  (void) reactor_net_construct(&tcpd.net, net_handler, &tcpd);
  reactor_net_set(&tcpd.net, REACTOR_NET_OPTION_REUSEADDR);
  e = reactor_net_bind(&tcpd.net, "127.0.0.1", "8080");
  if (e != REACTOR_OK)
    err(1, "reactor_net_bind");
  (void) reactor_run();

  reactor_destruct();
}
