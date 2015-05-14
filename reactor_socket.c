#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <sys/epoll.h>

#include "reactor.h"
#include "reactor_fd.h"
#include "reactor_socket.h"

reactor_socket *reactor_socket_new(reactor *r, reactor_handler *h, int descriptor, void *user)
{
  reactor_socket *s;
  int e;
  
  s = malloc(sizeof *s);
  e = reactor_socket_construct(r, s, h, descriptor, user);
  if (e == -1)
    {
      reactor_socket_delete(s);
      return NULL;
    }

  return s;
}

int reactor_socket_construct(reactor *r, reactor_socket *s, reactor_handler *h, int descriptor, void *data)
{
  int e;
  
  *s = (reactor_socket) {.user = {.handler = h, .data = data}, .reactor = r};
  e = reactor_fd_construct(r, &s->fd, reactor_socket_handler, descriptor, s);
  if (e == -1)
    return -1;
  
  return 0;
}

int reactor_socket_destruct(reactor_socket *s)
{
  return reactor_fd_destruct(&s->fd);
}

int reactor_socket_delete(reactor_socket *s)
{
  int e;
  
  e = reactor_socket_destruct(s);
  if (e == -1)
    return -1;

  free(s);
  return 0;
}

void reactor_socket_handler(reactor_event *e)
{
  reactor_socket *s = e->call->data;
  char buffer[4096];
  ssize_t n;
  
  while (1)
    {
      n = read(reactor_fd_descriptor(&s->fd), buffer, sizeof buffer);
      if (n == 0)
	{
	  reactor_dispatch(&s->user, REACTOR_SOCKET_CLOSE, NULL);
	  break;
	}
      
      if (n == -1 && errno == EAGAIN)
	break;

      if (n == -1)
	err(1, "read");

      reactor_dispatch(&s->user, REACTOR_SOCKET_DATA, (reactor_data[]){{.base = buffer, .size = n}});
    }
}
