#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "reactor_user.h"
#include "reactor_descriptor.h"
#include "reactor_core.h"

int reactor_descriptor_open(reactor_descriptor *descriptor, reactor_user_callback *callback, void *state, int fd,
                             int events)
{
  reactor_user_construct(&descriptor->user, callback, state);
  descriptor->state = REACTOR_DESCRIPTOR_STATE_OPEN;
  descriptor->fd = fd;
  return reactor_core_add(descriptor, events);
}

void reactor_descriptor_close(reactor_descriptor *descriptor)
{
  if (descriptor->state != REACTOR_DESCRIPTOR_STATE_OPEN)
    return;
  descriptor->state = REACTOR_DESCRIPTOR_STATE_CLOSED;
  (void) close(descriptor->fd);
  reactor_core_flush(descriptor);
}

int reactor_descriptor_clear(reactor_descriptor *descriptor)
{
  if (descriptor->state != REACTOR_DESCRIPTOR_STATE_OPEN)
    return REACTOR_ERROR;
  descriptor->state = REACTOR_DESCRIPTOR_STATE_CLOSED;
  reactor_core_delete(descriptor);
  return descriptor->fd;
}

int reactor_descriptor_event(reactor_descriptor *d, int events)
{
  return reactor_user_dispatch(&d->user, REACTOR_DESCRIPTOR_EVENT_POLL, &events);
}

void reactor_descriptor_set(reactor_descriptor *descriptor, int events)
{
  reactor_core_modify(descriptor, events);
}

int reactor_descriptor_fd(reactor_descriptor *d)
{
  return d->fd;
}
