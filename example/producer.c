#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <sys/epoll.h>
#include <sys/types.h>

#include <dynamic.h>

#include "reactor.h"

struct state
{
  reactor_fd fd;
};

reactor_status fd_event(reactor_event *event)
{
  struct state *state = event->state;
  int i;
  ssize_t n;

  switch (event->data)
    {
    case EPOLLIN:
      n = read(reactor_fd_fileno(&state->fd), &i, sizeof i);
      if (n != sizeof i)
        err(1, "read");
      printf("[receive] %d\n", i);
      break;
    default:
      reactor_fd_close(&state->fd);
      return REACTOR_ABORT;
    }

  return REACTOR_OK;
}

reactor_status pool_event(reactor_event *event)
{
  ssize_t n;
  int i;

  if (event->type == REACTOR_POOL_EVENT_RETURN)
    return REACTOR_OK;

  for (i = 0; i < 3; i ++)
    {
      n = write(*(int *) event->state, &i, sizeof i);
      if (n != sizeof i)
        err(1, "write");
      sleep(1);
    }

  (void) close(*(int *) event->state);
  return REACTOR_OK;
}

int main()
{
  struct state state;
  int e, fd[2];

  (void) reactor_construct();

  e = pipe2(fd, O_DIRECT | O_NONBLOCK);
  if (e == -1)
    err(1, "pipe2");

  reactor_fd_construct(&state.fd, fd_event, &state);
  (void) reactor_fd_open(&state.fd, fd[0], EPOLLIN);
  (void) reactor_pool_dispatch(pool_event, &fd[1]);
  (void) reactor_run();

  reactor_destruct();
}
