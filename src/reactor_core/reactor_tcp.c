#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_tcp.h"

static int reactor_tcp_bind(reactor_tcp *tcp, int s, struct sockaddr *sa, socklen_t sa_len)
{
  int e;

  e = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int));
  if (e == -1)
    return -1;

  e = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));
  if (e == -1)
    return -1;

  e = bind(s, sa, sa_len);
  if (e == -1)
    return -1;

  e = listen(s, -1);
  if (e == -1)
    return -1;

  e = reactor_desc_open(&tcp->desc, s);
  if (e == -1)
    return -1;

  return 0;
}

void reactor_tcp_init(reactor_tcp *tcp, reactor_user_callback *callback, void *state)
{
  *tcp = (reactor_tcp) {.state = REACTOR_TCP_CLOSED};
  reactor_user_init(&tcp->user, callback, state);
  reactor_desc_init(&tcp->desc, reactor_tcp_event, tcp);
}

void reactor_tcp_error(reactor_tcp *tcp)
{
  reactor_user_dispatch(&tcp->user, REACTOR_TCP_ERROR, NULL);
}

void reactor_tcp_close(reactor_tcp *tcp)
{
  if (tcp->state != REACTOR_TCP_OPEN)
      return;
  reactor_desc_close(&tcp->desc);
  tcp->state = REACTOR_TCP_CLOSED;
  reactor_user_dispatch(&tcp->user, REACTOR_TCP_CLOSE, NULL);
}

void reactor_tcp_connect(reactor_tcp *tcp, char *node, char *service)
{
  struct addrinfo *ai;
  int s, e;

  if (tcp->state != REACTOR_TCP_CLOSED)
    {
      reactor_tcp_error(tcp);
      return;
    }

  e = getaddrinfo(node, service,
                  (struct addrinfo[]) {{.ai_family = AF_INET, .ai_socktype = SOCK_STREAM}},
                  &ai);
  if (e != 0 || !ai)
    {
      reactor_tcp_error(tcp);
      return;
    }

  s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if (s == -1)
    {
      freeaddrinfo(ai);
      reactor_tcp_error(tcp);
      return;
    }

  e = connect(s, ai->ai_addr, ai->ai_addrlen);
  freeaddrinfo(ai);
  if (e == -1 && errno != EINPROGRESS)
    {
      (void) close(s);
      reactor_tcp_error(tcp);
      return;
    }

  tcp->state = REACTOR_TCP_OPEN;
  reactor_user_dispatch(&tcp->user, REACTOR_TCP_CONNECT, &s);
}

void reactor_tcp_listen(reactor_tcp *tcp, char *node, char *service)
{
  struct addrinfo *ai;
    int s, e;

  if (tcp->state != REACTOR_TCP_CLOSED)
    {
      reactor_tcp_error(tcp);
      return;
    }

  e = getaddrinfo(node, service,
                  (struct addrinfo[]) {{.ai_family = AF_INET, .ai_socktype = SOCK_STREAM, .ai_flags = AI_PASSIVE}},
                  &ai);
  if (e != 0 || !ai)
    {
      reactor_tcp_error(tcp);
      return;
    }

  s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if (s == -1)
    {
      freeaddrinfo(ai);
      reactor_tcp_error(tcp);
      return;
    }

  e = reactor_tcp_bind(tcp, s, ai->ai_addr, ai->ai_addrlen);
  freeaddrinfo(ai);
  if (e == -1)
    {
      (void) close(s);
      reactor_tcp_error(tcp);
      return;
    }

  tcp->state = REACTOR_TCP_OPEN;
}

void reactor_tcp_event(void *state, int type, void *data)
{
  reactor_tcp *tcp = state;
  int s;

  switch (type)
    {
    case REACTOR_DESC_READ:
      s = accept(*(int *) data, NULL, NULL);
      if (s == -1)
        {
          reactor_tcp_error(tcp);
          return;
        }
      reactor_user_dispatch(&tcp->user, REACTOR_TCP_ACCEPT, &s);
      break;
    }
}
