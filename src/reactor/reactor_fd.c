#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "../reactor.h"

void reactor_fd_construct(reactor_fd *fd, reactor_user_callback *callback, void *state)
{
  reactor_user_construct(&fd->user, callback, state);
  fd->fileno = -1;
}

void reactor_fd_destruct(reactor_fd *fd)
{
  reactor_fd_close(fd);
}

int reactor_fd_deconstruct(reactor_fd *fd)
{
  int fileno;

  if (!reactor_fd_active(fd))
    return -1;

  fileno = fd->fileno;
  reactor_core_delete(&fd->user, fd->fileno);
  fd->fileno = -1;
  return fileno;
}

void reactor_fd_open(reactor_fd *fd, int fileno, int events)
{
  fd->fileno = fileno;
  reactor_core_add(&fd->user, fileno, events);
}

void reactor_fd_events(reactor_fd *fd, int events)
{
  reactor_core_modify(&fd->user, fd->fileno, events);
}

void reactor_fd_close(reactor_fd *fd)
{
  if (reactor_fd_active(fd))
    {
      (void) close(fd->fileno);
      reactor_core_deregister(&fd->user);
      fd->fileno = -1;
    }
}

int reactor_fd_active(reactor_fd *fd)
{
  return fd->fileno >= 0;
}

int reactor_fd_fileno(reactor_fd *fd)
{
  return fd->fileno;
}
