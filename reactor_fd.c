#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <sys/epoll.h>

#include "reactor.h"
#include "reactor_fd.h"

reactor_fd *reactor_fd_new(reactor *r, reactor_handler *h, int fd, void *user)
{
  reactor_fd *d;
  int e;
  
  d = malloc(sizeof *d);
  e = reactor_fd_construct(r, d, h, fd, user);
  if (e == -1)
    {
      (void) reactor_fd_destruct(d);
      return NULL;
    }
  
  return d;
}

int reactor_fd_construct(reactor *r, reactor_fd *d, reactor_handler *h, int fd, void *data)
{
  int e;
  
  *d = (reactor_fd) {.user = {.handler = h, .data = data},
		     .reactor = r, .descriptor = fd,
		     .ev = {.events = EPOLLIN | EPOLLET, .data.ptr = &d->main},
		     .main = {.handler = reactor_fd_handler, .data = d}};

  e = fcntl(fd, F_SETFL, O_NONBLOCK);
  if (e == -1)
    return -1;

  return epoll_ctl(r->epollfd, EPOLL_CTL_ADD, fd, &d->ev);
}

int reactor_fd_destruct(reactor_fd *d)
{
  int e;

  if (d->descriptor >= 0)
    {
      e = epoll_ctl(d->reactor->epollfd, EPOLL_CTL_DEL, d->descriptor, &d->ev);
      if (e == -1)
	return -1;
      
      e = close(d->descriptor);
      if (e == -1)
	return -1;
      
      d->descriptor = -1;
    }
  
  return 0;
}

int reactor_fd_delete(reactor_fd *d)
{
  int e;
  
  e = reactor_fd_destruct(d);
  if (e == -1)
    return -1;
  
  free(d);
  return 0;
}

void reactor_fd_handler(reactor_event *e)
{
  reactor_fd *d = e->call->data;
  int mask =
    (e->type & EPOLLIN ? REACTOR_FD_READ : 0) |
    (e->type & EPOLLOUT ? REACTOR_FD_WRITE : 0);
  
  reactor_dispatch(&d->user, mask, NULL);
}

int reactor_fd_descriptor(reactor_fd *d)
{
  return d->descriptor;
}

int reactor_fd_events(reactor_fd *d, int mask)
{
  d->ev.events =
    (mask & REACTOR_FD_READ ? EPOLLIN : 0) |
    (mask & REACTOR_FD_WRITE ? EPOLLOUT : 0) |
    EPOLLET;

  return epoll_ctl(d->reactor->epollfd, EPOLL_CTL_MOD, d->descriptor, &d->ev);
}
