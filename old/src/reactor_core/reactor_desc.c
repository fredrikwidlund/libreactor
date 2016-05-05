#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"

void reactor_desc_init(reactor_desc *desc, reactor_user_call *call, void *state)
{
  *desc = (reactor_desc) {.state = REACTOR_DESC_CLOSED};
  reactor_user_init(&desc->user, call, state);
}

int reactor_desc_open(reactor_desc *desc, int fd)
{
  int e;

  e = fcntl(fd, F_SETFL, O_NONBLOCK);
  if (e == -1)
    return -1;

  desc->fd = fd;
  desc->state = REACTOR_DESC_OPEN;
  desc->events = REACTOR_DESC_READ;
  desc->events_saved = desc->events;

  return reactor_core_add(desc->fd, desc->events, desc);
}

int reactor_desc_fd(reactor_desc *desc)
{
  return desc->fd;
}

void reactor_desc_events(reactor_desc *desc, int events)
{
  desc->events = events;
  reactor_desc_update(desc);
}

void reactor_desc_event(reactor_desc *desc, int events)
{
  reactor_user_dispatch(&desc->user, events, desc);
}

int reactor_desc_update(reactor_desc *desc)
{
  if (desc->events == desc->events_saved || desc->state != REACTOR_DESC_OPEN)
    return 0;

  desc->events_saved = desc->events;
  return reactor_core_mod(desc->fd, desc->events, desc);
}

void reactor_desc_close(reactor_desc *desc)
{
  if (desc->state == REACTOR_DESC_OPEN)
    {
      desc->state = REACTOR_DESC_CLOSED;
      reactor_core_del(desc->fd);
      reactor_core_schedule(reactor_desc_call, desc);
    }
}

void reactor_desc_scheduled_close(reactor_desc *desc)
{
  if (desc->state == REACTOR_DESC_OPEN)
    {
      desc->state = REACTOR_DESC_CLOSED;
      reactor_core_del(desc->fd);
    }
}

void reactor_desc_call(void *state, int type, void *data)
{
  reactor_desc *desc;

  (void) type;
  (void) data;

  desc = state;
  reactor_user_dispatch(&desc->user, REACTOR_DESC_CLOSE, desc);
}
