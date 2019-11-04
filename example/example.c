#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <err.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "reactor.h"

struct state
{
  reactor_fd fd;
  reactor_timer timer;
};

reactor_status fd_handler(reactor_event *event)
{
  struct state *s = event->state;

  printf("[fd] %p %d %lu\n", event->state, event->type, event->data);
  reactor_fd_close(&s->fd);
  reactor_timer_clear(&s->timer);
  return REACTOR_ABORT;
}

reactor_status timer_handler(reactor_event *event)
{
  printf("[timer] %p %d %lu\n", event->state, event->type, event->data);
  return REACTOR_OK;
}

reactor_status pool_handler(reactor_event *event)
{
  if (event->type == REACTOR_POOL_EVENT_CALL)
    printf("[pool call] %p %d %lu\n", event->state, event->type, event->data);
  else if (event->type == REACTOR_POOL_EVENT_RETURN)
    printf("[pool return] %p %d %lu\n", event->state, event->type, event->data);
  return REACTOR_OK;
}

int main()
{
  int i;
  struct state s;

  reactor_construct();
  reactor_fd_construct(&s.fd, fd_handler, &s);
  reactor_fd_open(&s.fd, 0, EPOLLIN | EPOLLET);

  reactor_timer_construct(&s.timer, timer_handler, &s);
  reactor_timer_set(&s.timer, 1, 1000000000);

  for (i = 0; i < 16; i ++)
    reactor_pool_dispatch(pool_handler, NULL);

  reactor_run();
  reactor_destruct();
}
