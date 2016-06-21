#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/eventfd.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_event.h"

static inline void reactor_event_close_final(reactor_event *event)
{
  if (event->state == REACTOR_EVENT_CLOSE_WAIT &&
      event->desc.state == REACTOR_DESC_CLOSED &&
      event->ref == 0)
    {
      event->state = REACTOR_EVENT_CLOSED;
      reactor_user_dispatch(&event->user, REACTOR_EVENT_CLOSE, NULL);
    }
}

static inline void reactor_event_hold(reactor_event *event)
{
  event->ref ++;
}

static inline void reactor_event_release(reactor_event *event)
{
  event->ref --;
  reactor_event_close_final(event);
}

void reactor_event_init(reactor_event *event, reactor_user_callback *callback, void *state)
{
  *event = (reactor_event) {.state = REACTOR_EVENT_CLOSED};
  reactor_user_init(&event->user, callback, state);
  reactor_desc_init(&event->desc, reactor_event_event, event);
}

void reactor_event_open(reactor_event *event)
{
  int fd;

  if (event->state != REACTOR_EVENT_CLOSED)
    {
      reactor_event_error(event);
      return;
    }
  event->state = REACTOR_EVENT_OPEN;
  //fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
  fd = eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE);
  if (fd == -1)
    {
      reactor_event_error(event);
      return;
    }

  reactor_desc_open(&event->desc, fd, REACTOR_DESC_FLAGS_READ);
}

void reactor_event_error(reactor_event *event)
{
  event->state = REACTOR_EVENT_INVALID;
  reactor_user_dispatch(&event->user, REACTOR_EVENT_ERROR, NULL);
}

void reactor_event_close(reactor_event *event)
{
  if (event->state == REACTOR_EVENT_CLOSED)
    return;
  event->state = REACTOR_EVENT_CLOSE_WAIT;
  reactor_desc_close(&event->desc);
  reactor_event_close_final(event);
}

void reactor_event_event(void *state, int type, void *data)
{
  reactor_event *event = state;
  uint64_t value;
  ssize_t n;

  (void) data;
  switch(type)
    {
    case REACTOR_DESC_READ:
      n = read(reactor_desc_fd(&event->desc), &value, sizeof value);
      if (n != sizeof value)
        break;
      reactor_user_dispatch(&event->user, REACTOR_EVENT_SIGNAL, &value);
      break;
    case REACTOR_DESC_ERROR:
      reactor_event_error(event);
      break;
    case REACTOR_DESC_SHUTDOWN:
      reactor_user_dispatch(&event->user, REACTOR_EVENT_SHUTDOWN, &value);
      break;
    case REACTOR_DESC_CLOSE:
      reactor_event_close_final(event);
      break;
    }
}
