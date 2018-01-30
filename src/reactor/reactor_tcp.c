#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <err.h>

#include <dynamic.h>

#include "reactor_util.h"
#include "reactor_user.h"
#include "reactor_core.h"
#include "reactor_descriptor.h"
#include "reactor_resolver.h"
#include "reactor_tcp.h"

static int reactor_tcp_configure(reactor_tcp *, struct addrinfo *);

static int reactor_tcp_error(reactor_tcp *tcp)
{
  return reactor_user_dispatch(&tcp->user, REACTOR_TCP_EVENT_ERROR, NULL);
}

static int reactor_tcp_resolver_event(void *state, int type, void *data)
{
  reactor_tcp *tcp = state;
  int e;

  (void) type;
  if (!data)
    return reactor_tcp_error(tcp);

  e = reactor_tcp_configure(tcp, data);
  if (e == REACTOR_ERROR)
    return reactor_tcp_error(tcp);

  return REACTOR_OK;
}

static int reactor_tcp_client_event(void *state, int type, void *data)
{
  reactor_tcp *tcp = state;
  int fd, flags = *(int *) data;

  (void) type;
  if (flags != EPOLLOUT)
    return reactor_tcp_error(tcp);

  fd = reactor_descriptor_clear(&tcp->descriptor);
  return reactor_user_dispatch(&tcp->user, REACTOR_TCP_EVENT_CONNECT, &fd);
}

static int reactor_tcp_server_event(void *state, int type, void *data)
{
  reactor_tcp *tcp = state;
  int fd, e, flags = *(int *) data;

  (void) type;
  if (flags != EPOLLIN)
    return reactor_tcp_error(tcp);

  while (1)
    {
      fd = accept(reactor_descriptor_fd(&tcp->descriptor), NULL, NULL);
      if (fd == -1)
        return errno == EAGAIN ? REACTOR_OK : reactor_tcp_error(tcp);
      e = reactor_user_dispatch(&tcp->user, REACTOR_TCP_EVENT_ACCEPT, &fd);
      if (e != REACTOR_OK)
        return REACTOR_ABORT;
    }
}

static int reactor_tcp_configure(reactor_tcp *tcp, struct addrinfo *ai)
{
  int e, fd;

  fd = socket(ai->ai_family, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (fd == -1)
    return REACTOR_ERROR;

  if (tcp->flags & REACTOR_TCP_FLAG_SERVER)
    {
      (void) setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int));
      (void) setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));

      e = bind(fd, ai->ai_addr, ai->ai_addrlen);
      if (e == -1)
        {
          (void) close(fd);
          return REACTOR_ERROR;
        }

      e = listen(fd, -1);
      if (e == -1)
        {
          (void) close(fd);
          return REACTOR_ERROR;
        }

      return reactor_descriptor_open(&tcp->descriptor, reactor_tcp_server_event, tcp, fd, EPOLLIN | EPOLLOUT | EPOLLET);
    }
  else
    {
      e = connect(fd, ai->ai_addr, ai->ai_addrlen);
      if (e == -1 && errno != EINPROGRESS)
        {
          (void) close(fd);
          return REACTOR_ERROR;
        }

      return reactor_descriptor_open(&tcp->descriptor, reactor_tcp_client_event, tcp, fd, EPOLLOUT | EPOLLET);
    }
}

int reactor_tcp_open(reactor_tcp *tcp, reactor_user_callback *callback, void *state,
                     char *node, char *service, int flags)
{
  *tcp = (reactor_tcp) {.flags = flags};
  reactor_user_construct(&tcp->user, callback, state);
  return reactor_resolver_open(&tcp->resolver, reactor_tcp_resolver_event, tcp, node, service,
                               AF_INET, SOCK_STREAM, flags == REACTOR_TCP_FLAG_SERVER ? AI_PASSIVE : 0);
}

int reactor_tcp_open_addrinfo(reactor_tcp *tcp, reactor_user_callback *callback, void *state,
                              struct addrinfo *addrinfo, int flags)
{
  *tcp = (reactor_tcp) {.flags = flags};
  reactor_user_construct(&tcp->user, callback, state);
  return reactor_tcp_configure(tcp, addrinfo);
}

void reactor_tcp_close(reactor_tcp *tcp)
{
  reactor_resolver_close(&tcp->resolver);
  reactor_descriptor_close(&tcp->descriptor);
}
