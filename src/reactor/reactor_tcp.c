#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "reactor_user.h"
#include "reactor_core.h"
#include "reactor_tcp.h"

static void reactor_tcp_close_fd(reactor_tcp *tcp)
{
  if (tcp->fd >= 0)
    {
      reactor_core_deregister(tcp->fd);
      (void) close(tcp->fd);
      tcp->fd = -1;
    }
}

static void reactor_tcp_error(reactor_tcp *tcp)
{
  tcp->state = REACTOR_TCP_STATE_ERROR;
  reactor_user_dispatch(&tcp->user, REACTOR_TCP_EVENT_ERROR, NULL);
}

static void reactor_tcp_listen(reactor_tcp *tcp)
{
  (void) tcp;
}

static void reactor_tcp_resolve(reactor_tcp *tcp)
{
  struct addrinfo hints, *ai;
  int e;

  hints = (struct addrinfo) {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM,
                             .ai_flags = AI_NUMERICHOST | AI_NUMERICSERV};
  e = getaddrinfo(tcp->node, tcp->service, &hints, &ai);
  if (e == -1 || !ai)
    reactor_tcp_error(tcp);
  else
    reactor_tcp_listen(tcp);
}

void reactor_tcp_hold(reactor_tcp *tcp)
{
  tcp->ref ++;
}

void reactor_tcp_release(reactor_tcp *tcp)
{
  tcp->ref --;
  if (!tcp->ref)
    {
      tcp->state = REACTOR_TCP_STATE_CLOSED;
      reactor_user_dispatch(&tcp->user, REACTOR_TCP_EVENT_CLOSE, NULL);
    }
}

void reactor_tcp_open(reactor_tcp *tcp, reactor_user_callback *callback, void *state,
                      char *node, char *service, int flags)
{
  tcp->ref = 0;
  tcp->state = REACTOR_TCP_STATE_RESOLVING;
  reactor_user_construct(&tcp->user, callback, state);
  tcp->fd = -1;
  tcp->flags = flags;
  tcp->node = strdup(node);
  tcp->service = strdup(service);
  reactor_tcp_hold(tcp);
  reactor_tcp_resolve(tcp);
}

void reactor_tcp_close(reactor_tcp *tcp)
{
  if (tcp->state & (REACTOR_TCP_STATE_RESOLVING | REACTOR_TCP_STATE_OPEN | REACTOR_TCP_STATE_ERROR))
    {
      reactor_tcp_close_fd(tcp);
      tcp->state = REACTOR_TCP_STATE_CLOSING;
      reactor_tcp_release(tcp);
    }
}
