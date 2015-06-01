#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <sys/epoll.h>

#include "buffer.h"
#include "vector.h"
#include "reactor_core.h"
#include "reactor_fd.h"

reactor_fd *reactor_fd_new(reactor *r, int descriptor, reactor_handler *handler, void *state)
{
  reactor_fd *fd;
  int e;
  
  fd = malloc(sizeof *fd);
  e = reactor_fd_init(r, fd, descriptor, handler, state);
  fd->flags |= REACTOR_FD_FLAG_ALLOCATED;
  if (e == -1)
    {
      (void) reactor_fd_delete(fd);
      return NULL;
    }

  return fd;
}

int reactor_fd_init(reactor *r, reactor_fd *fd, int descriptor, reactor_handler *handler, void *state)
{
  int e;
  
  *fd = (reactor_fd) {.descriptor = descriptor,
		      .reactor = r,
		      .ev = {.events = EPOLLIN | EPOLLET, .data.ptr = &fd->main},
		      .user = {.handler = handler, .state = state},
		      .main = {.handler = reactor_fd_handler, .state = fd}};
  e = fcntl(fd->descriptor, F_SETFL, O_NONBLOCK);
  if (e == -1)
    return -1;

  e = epoll_ctl(r->epollfd, EPOLL_CTL_ADD, fd->descriptor, &fd->ev);
  if (e == -1)
    return -1;

  fd->flags |= REACTOR_FD_FLAG_ACTIVE;
  return 0;
}

int reactor_fd_destruct(reactor_fd *fd)
{
  int e;

  if (fd->descriptor >= 0)
    {
      e = epoll_ctl(fd->reactor->epollfd, EPOLL_CTL_DEL, fd->descriptor, &fd->ev);
      if (e == -1)
	return -1;
      
      e = close(fd->descriptor);
      if (e == -1)
	return -1;
      
      fd->descriptor = -1;
    }
  
  return 0;
}

int reactor_fd_delete(reactor_fd *fd)
{
  int e, allocated;

  if (fd->reactor->flags & REACTOR_FLAG_EVENT)
    {
      if (fd->flags & REACTOR_FLAG_DELETE)
	return 0;

      fd->flags &= ~(REACTOR_FD_FLAG_ACTIVE | REACTOR_FLAG_DELETE);
      return reactor_schedule(fd->reactor, &fd->main);
    }

  if (fd->descriptor >= 0)
    {
      e = epoll_ctl(fd->reactor->epollfd, EPOLL_CTL_DEL, fd->descriptor, &fd->ev);
      if (e == -1)
	return -1;
      
      e = close(fd->descriptor);
      if (e == -1)
	return -1;
    }

  allocated = fd->flags & REACTOR_FD_FLAG_ALLOCATED;
  reactor_dispatch(fd, &fd->user, REACTOR_FD_DELETE, NULL);
  if (allocated)
    free(fd);
  
  return 0;
}

void reactor_fd_handler(void *state, reactor_event *event)
{
  reactor_fd *fd = state;
  int events;
  
  switch (event->type)
    {
    case REACTOR_EVENT:
      events = *(int *) event->data;
      if (events & EPOLLIN && fd->flags & REACTOR_FD_FLAG_ACTIVE)
	reactor_dispatch(fd, &fd->user, REACTOR_FD_READ, NULL);
      
      if (events & EPOLLERR && fd->flags & REACTOR_FD_FLAG_ACTIVE)
	  reactor_dispatch(fd, &fd->user, REACTOR_FD_ERROR, NULL);
  
      if (events & EPOLLOUT && fd->flags & REACTOR_FD_FLAG_ACTIVE)
	reactor_dispatch(fd, &fd->user, REACTOR_FD_WRITE, NULL);
      break;
    case REACTOR_SCHEDULED_CALL:
      reactor_fd_delete(fd);
      break;
    }
}

/*
int reactor_fd_events(reactor_fd *fd, int mask)
{
  d->ev.events =
    (mask & REACTOR_FD_READ ? EPOLLIN : 0) |
    (mask & REACTOR_FD_WRITE ? EPOLLOUT : 0) |
    EPOLLET;

  return epoll_ctl(d->reactor->epollfd, EPOLL_CTL_MOD, d->descriptor, &d->ev);
}
*/
