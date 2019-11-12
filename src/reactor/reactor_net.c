#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "../reactor.h"

static reactor_status reactor_net_error(reactor_net *net, char *description)
{
  return reactor_user_dispatch(&net->user, REACTOR_NET_EVENT_ERROR, (uintptr_t) description);
}

static reactor_status reactor_net_fd_handler(reactor_event *event)
{
  reactor_net *net = event->state;
  int fd, e;

  switch (event->data)
    {
    case EPOLLIN:
      while (1)
        {
          fd = accept4(reactor_fd_fileno(&net->fd), NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
          if (fd == -1)
            return errno == EAGAIN ? REACTOR_OK : reactor_net_error(net, "error accepting socket");
          e = reactor_user_dispatch(&net->user, REACTOR_NET_EVENT_ACCEPT, fd);
          if (e != REACTOR_OK)
            return e;
        }
      return REACTOR_OK;
    case EPOLLOUT:
      return reactor_user_dispatch(&net->user, REACTOR_NET_EVENT_CONNECT, reactor_fd_deconstruct(&net->fd));
    default:
      return reactor_net_error(net, "descriptor error");
    }
}

static reactor_status reactor_net_resolver_handler(reactor_event *event)
{
  reactor_net *net = event->state;
  struct addrinfo *ai = (struct addrinfo *) event->data;
  int e, fileno;

  net->resolver_job = 0;

  if (!ai)
    return reactor_net_error(net, "error resolving address");

  fileno = socket(ai->ai_family, SOCK_STREAM | SOCK_NONBLOCK, 0);
  reactor_assert_int_not_equal(fileno, -1);

 if (net->options & REACTOR_NET_OPTION_PASSIVE)
    {
      if (net->options & REACTOR_NET_OPTION_REUSEPORT)
        (void) setsockopt(fileno, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int));
      if (net->options & REACTOR_NET_OPTION_REUSEADDR)
        (void) setsockopt(fileno, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));

      e = bind(fileno, ai->ai_addr, ai->ai_addrlen);
      if (e == -1)
        {
          (void) close(fileno);
          return reactor_net_error(net, "error binding socket");
        }

      e = listen(fileno, INT_MAX);
      reactor_assert_int_not_equal(e, -1);
      reactor_fd_open(&net->fd, fileno, EPOLLIN | EPOLLET);
    }
  else
    {
     e = connect(fileno, ai->ai_addr, ai->ai_addrlen);
      if (e == -1 && errno != EINPROGRESS)
        {
          (void) close(fileno);
          return reactor_net_error(net, "error connecting socket");
        }

      reactor_fd_open(&net->fd, fileno, EPOLLOUT | EPOLLET);
    }

  return REACTOR_OK;
}

void reactor_net_construct(reactor_net *net, reactor_user_callback *callback, void *state)
{
  reactor_user_construct(&net->user, callback, state);
  reactor_fd_construct(&net->fd, reactor_net_fd_handler, net);
  net->resolver_job = 0;
  net->options = REACTOR_NET_OPTION_DEFAULTS;
}

void reactor_net_destruct(reactor_net *net)
{
  reactor_net_reset(net);
}

void reactor_net_reset(reactor_net *net)
{
  reactor_fd_close(&net->fd);
  if (net->resolver_job)
    {
      reactor_resolver_cancel(net->resolver_job);
      net->resolver_job = 0;
    }
  net->options = REACTOR_NET_OPTION_DEFAULTS;
}

void reactor_net_set(reactor_net *net, reactor_net_options options)
{
  net->options |= options;
}

void reactor_net_clear(reactor_net *net, reactor_net_options options)
{
  net->options &= ~options;
}

reactor_status reactor_net_bind(reactor_net *net, char *node, char *service)
{
  if (reactor_fd_active(&net->fd))
    return REACTOR_ERROR;

  net->options |= REACTOR_NET_OPTION_PASSIVE;
  net->resolver_job = reactor_resolver_request(reactor_net_resolver_handler, net, node, service, 0, SOCK_STREAM, AI_PASSIVE);
  return net->resolver_job ? REACTOR_OK : REACTOR_ERROR;
}

reactor_status reactor_net_connect(reactor_net *net, char *node, char *service)
{
  if (reactor_fd_active(&net->fd))
    return REACTOR_ERROR;

  net->resolver_job = reactor_resolver_request(reactor_net_resolver_handler, net, node, service, 0, SOCK_STREAM, 0);
  return net->resolver_job ? REACTOR_OK : REACTOR_ERROR;
}
