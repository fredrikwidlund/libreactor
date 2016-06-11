#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"

static __thread reactor_core core = {.state = REACTOR_CORE_CLOSED};

int reactor_core_open(void)
{
  if (core.state != REACTOR_CORE_CLOSED)
    return -1;

  vector_init(&core.polls, sizeof(struct pollfd));
  vector_init(&core.descs, sizeof(reactor_desc *));
  core.state = REACTOR_CORE_OPEN;
  return 0;
}

int reactor_core_run(void)
{
  struct pollfd *p;
  size_t i;
  int e;

  core.state = REACTOR_CORE_RUNNING;
  while (core.state == REACTOR_CORE_RUNNING && vector_size(&core.polls))
    {
      e = poll(vector_data(&core.polls), vector_size(&core.polls), -1);
      if (e == -1)
        return -1;

      i = 0;
      while (i < vector_size(&core.polls))
        {
          p = vector_at(&core.polls, i);
          core.current = p->fd;
          if (p->revents)
            reactor_desc_event(*(reactor_desc **) vector_at(&core.descs, i), p->revents, NULL);
          if (core.current != -1)
            i ++;
        }
    }
  core.state = REACTOR_CORE_OPEN;
  return 0;
}

void reactor_core_close(void)
{
  vector_clear(&core.polls);
  vector_clear(&core.descs);
  core.state = REACTOR_CORE_CLOSED;
}

int reactor_core_desc_add(reactor_desc *desc, int fd, int events)
{
  int e;

  if (fd < 0)
    return -1;

  e = vector_push_back(&core.polls, (struct pollfd[]) {{.fd = fd, .events = events}});
  if (e == -1)
    return -1;

  e = vector_push_back(&core.descs, &desc);
  if (e == -1)
    {
      vector_pop_back(&core.polls);
      return -1;
    }

  desc->index = vector_size(&core.polls) - 1;
  return 0;
}

void reactor_core_desc_set(reactor_desc *desc, int events)
{
  ((struct pollfd *) vector_at(&core.polls, desc->index))->events |= events;
}

void reactor_core_desc_clear(reactor_desc *desc, int events)
{
  ((struct pollfd *) vector_at(&core.polls, desc->index))->events &= ~events;
}

int reactor_core_desc_fd(reactor_desc *desc)
{
  return ((struct pollfd *) vector_at(&core.polls, desc->index))->fd;
}

void reactor_core_desc_remove(reactor_desc *desc)
{
  int last;

  if (((struct pollfd *) vector_at(&core.polls, desc->index))->fd == core.current)
    core.current = -1;
  last = vector_size(&core.polls) - 1;
  if (desc->index != last)
    {
      memcpy(vector_at(&core.polls, desc->index), vector_at(&core.polls, last), sizeof(struct pollfd));
      memcpy(vector_at(&core.descs, desc->index), vector_at(&core.descs, last), sizeof(reactor_desc *));
      (*(reactor_desc **) vector_at(&core.descs, desc->index))->index = desc->index;
    }

  vector_pop_back(&core.polls);
  vector_pop_back(&core.descs);
}
