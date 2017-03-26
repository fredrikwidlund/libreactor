#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_pool.h"
#include "reactor_core.h"
#include "reactor_resolver.h"
#include "reactor_tcp.h"

static void reactor_tcp_close_fd(reactor_tcp *tcp)
{
  if (tcp->socket >= 0)
    {
      reactor_core_fd_deregister(tcp->socket);
      (void) close(tcp->socket);
      tcp->socket = -1;
    }
}

static void reactor_tcp_error(reactor_tcp *tcp)
{
  tcp->state = REACTOR_TCP_STATE_ERROR;
  reactor_user_dispatch(&tcp->user, REACTOR_TCP_EVENT_ERROR, NULL);
}

static void reactor_tcp_socket_event(void *state, int type, void *data)
{
  reactor_tcp *tcp = state;
  short revents = ((struct pollfd *) data)->revents;
  int s;

  (void) type;
  if (revents & (POLLERR | POLLNVAL | POLLHUP))
    reactor_tcp_error(tcp);
  else if (revents & POLLIN)
    {
      s = accept(tcp->socket, NULL, NULL);
      if (s >= 0)
        reactor_user_dispatch(&tcp->user, REACTOR_TCP_EVENT_ACCEPT, &s);
      else
        reactor_tcp_error(tcp);
    }
  else if (revents & POLLOUT)
    reactor_user_dispatch(&tcp->user, REACTOR_TCP_EVENT_CONNECT, &tcp->socket);
}

static void reactor_tcp_connect(reactor_tcp *tcp, struct addrinfo *addrinfo)
{
  int s, e;

  s = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
  if (s == -1)
    {
      reactor_tcp_error(tcp);
      return;
    }

  e = fcntl(s, F_SETFL, O_NONBLOCK);
  if (e == -1)
    {
      (void) close(s);
      reactor_tcp_error(tcp);
      return;
    }

  e = connect(s, addrinfo->ai_addr, addrinfo->ai_addrlen);
  if (e == -1 && errno != EINPROGRESS)
    {
      (void) close(s);
      reactor_tcp_error(tcp);
      return;
    }

  tcp->socket = s;
  reactor_core_fd_register(tcp->socket, reactor_tcp_socket_event, tcp, POLLOUT);
}


static void reactor_tcp_listen(reactor_tcp *tcp, struct addrinfo *addrinfo)
{
  int s, e;

  s = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
  if (s == -1)
    {
      reactor_tcp_error(tcp);
      return;
    }

  (void) setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int));
  (void) setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));

  e = bind(s, addrinfo->ai_addr, addrinfo->ai_addrlen);
  if (e == -1)
    {
      (void) close(s);
      reactor_tcp_error(tcp);
      return;
    }

  e = listen(s, -1);
  if (e == -1)
    {
      (void) close(s);
      reactor_tcp_error(tcp);
      return;
    }

  tcp->socket = s;
  reactor_core_fd_register(tcp->socket, reactor_tcp_socket_event, tcp, POLLIN);
}

static void reactor_tcp_resolve_event(void *state, int type, void *data)
{
  reactor_tcp *tcp = state;

  switch (type)
    {
    case REACTOR_RESOLVER_EVENT_RESULT:
      if (!data)
        reactor_tcp_error(tcp);
      else if (tcp->flags & REACTOR_TCP_FLAG_SERVER)
        reactor_tcp_listen(tcp, data);
      else
        reactor_tcp_connect(tcp, data);
      break;
    case REACTOR_RESOLVER_EVENT_ERROR:
      reactor_tcp_error(tcp);
      break;
    case REACTOR_RESOLVER_EVENT_CLOSE:
      free(tcp->resolver);
      tcp->resolver = NULL;
      reactor_tcp_release(tcp);
      break;
    }
}

static void reactor_tcp_resolve(reactor_tcp *tcp, char *node, char *service)
{
  reactor_tcp_hold(tcp);
  tcp->resolver = malloc(sizeof *tcp->resolver);
  reactor_resolver_open(tcp->resolver, reactor_tcp_resolve_event, tcp, node, service, NULL);
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
  tcp->socket = -1;
  tcp->flags = flags;
  reactor_tcp_hold(tcp);
  reactor_tcp_resolve(tcp, node, service);
}

void reactor_tcp_close(reactor_tcp *tcp)
{
  if (tcp->state & (REACTOR_TCP_STATE_CLOSED | REACTOR_TCP_STATE_CLOSING))
    return;

  tcp->state = REACTOR_TCP_STATE_CLOSING;
  reactor_tcp_close_fd(tcp);
  reactor_tcp_release(tcp);
}
