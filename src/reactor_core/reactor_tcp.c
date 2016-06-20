#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_tcp.h"

static void reactor_tcp_close_final(reactor_tcp *tcp)
{
  tcp->state = REACTOR_TCP_CLOSED;
  reactor_user_dispatch(&tcp->user, REACTOR_TCP_CLOSE, NULL);
}

void reactor_tcp_init(reactor_tcp *tcp, reactor_user_callback *callback, void *state)
{
  *tcp = (reactor_tcp) {.state = REACTOR_TCP_CLOSED};
  reactor_user_init(&tcp->user, callback, state);
  reactor_desc_init(&tcp->desc, reactor_tcp_event, tcp);
}

void reactor_tcp_error(reactor_tcp *tcp)
{
  tcp->state = REACTOR_TCP_INVALID;
  reactor_user_dispatch(&tcp->user, REACTOR_TCP_ERROR, NULL);
}

void reactor_tcp_close(reactor_tcp *tcp)
{
  if (tcp->state != REACTOR_TCP_OPEN && tcp->state != REACTOR_TCP_INVALID)
      return;

  reactor_desc_close(&tcp->desc);
}

static void reactor_tcp_listen(reactor_tcp *tcp, struct addrinfo *ai, int s)
{
  int e;

  (void) setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int));
  (void) setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));

  e = bind(s, ai->ai_addr, ai->ai_addrlen);
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

  reactor_desc_open(&tcp->desc, s, REACTOR_DESC_FLAGS_READ);
}

static void reactor_tcp_connect(reactor_tcp *tcp, struct addrinfo *ai, int s)
{
  int e;

  e = connect(s, ai->ai_addr, ai->ai_addrlen);
  if (e == -1 && errno != EINPROGRESS)
    {
      (void) close(s);
      reactor_tcp_error(tcp);
      return;
    }

  reactor_desc_open(&tcp->desc, s, REACTOR_DESC_FLAGS_WRITE);
}

static void reactor_tcp_socket(reactor_tcp *tcp, struct addrinfo *ai)
{
  int s, e;

  s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
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

  (tcp->flags & REACTOR_TCP_SERVER ? reactor_tcp_listen : reactor_tcp_connect)(tcp, ai, s);
}

static void reactor_tcp_resolve(reactor_tcp *tcp, char *node, char *service)
{
  struct addrinfo *ai, hints = {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
  int e;

  hints.ai_flags = tcp->flags & REACTOR_TCP_SERVER ? AI_PASSIVE : 0;
  e = getaddrinfo(node, service, &hints, &ai);
  if (e != 0 || !ai)
    {
      reactor_tcp_error(tcp);
      return;
    }

  reactor_tcp_socket(tcp, ai);
  freeaddrinfo(ai);
}

void reactor_tcp_open(reactor_tcp *tcp, char *node, char *service, int flags)
{
  if (tcp->state != REACTOR_TCP_CLOSED)
    {
      reactor_tcp_error(tcp);
      return;
    }
  tcp->flags = flags;
  tcp->state = REACTOR_TCP_OPEN;
  reactor_tcp_resolve(tcp, node, service);
}

void reactor_tcp_event(void *state, int type, void *data)
{
  reactor_tcp *tcp = state;
  int s;

  (void) data;
  switch (type)
    {
    case REACTOR_DESC_WRITE:
      s = dup(reactor_desc_fd(&tcp->desc));
      reactor_desc_close(&tcp->desc);
      reactor_user_dispatch(&tcp->user, REACTOR_TCP_CONNECT, &s);
      break;
    case REACTOR_DESC_READ:
      s = accept(reactor_desc_fd(&tcp->desc), NULL, NULL);
      if (s == -1)
        {
          reactor_tcp_error(tcp);
          return;
        }
      reactor_user_dispatch(&tcp->user, REACTOR_TCP_ACCEPT, &s);
      break;
    case REACTOR_DESC_SHUTDOWN:
      reactor_user_dispatch(&tcp->user, REACTOR_TCP_ERROR, NULL);
      break;
    case REACTOR_DESC_CLOSE:
      reactor_tcp_close_final(tcp);
      break;
    }
}
