#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <assert.h>

#include "reactor.h"
#include "descriptor.h"

static uint32_t descriptor_events(descriptor *descriptor)
{
  return
    EPOLLHUP |
    EPOLLRDHUP |
    (descriptor->mask & DESCRIPTOR_READ ? EPOLLIN : 0) |
    (descriptor->mask & DESCRIPTOR_WRITE ? EPOLLOUT : 0) |
    (descriptor->mask & DESCRIPTOR_LEVEL ? 0 : EPOLLET);
}

static void descriptor_callback(reactor_event *event)
{
  descriptor *descriptor = event->state;
  struct epoll_event *e = (struct epoll_event *) event->data;

  assert(event->type == REACTOR_EPOLL_EVENT);
  if (e->events & EPOLLIN)
    reactor_dispatch(&descriptor->handler, DESCRIPTOR_READ, 0);
  if (e->events & EPOLLOUT)
    reactor_dispatch(&descriptor->handler, DESCRIPTOR_WRITE, 0);
  if (e->events & ~(EPOLLIN | EPOLLOUT))
    reactor_dispatch(&descriptor->handler, DESCRIPTOR_CLOSE, 0);
}

void descriptor_construct(descriptor *descriptor, reactor_callback *callback, void *state)
{
  reactor_handler_construct(&descriptor->handler, callback, state);
  reactor_handler_construct(&descriptor->epoll_handler, descriptor_callback, descriptor);
  descriptor->fd = -1;
}

void descriptor_destruct(descriptor *descriptor)
{
  descriptor_close(descriptor);
  reactor_handler_destruct(&descriptor->handler);
  reactor_handler_destruct(&descriptor->epoll_handler);
}

void descriptor_open(descriptor *descriptor, int fd, enum descriptor_mask mask)
{
  descriptor->fd = fd;
  descriptor->mask = mask;
  reactor_add(&descriptor->epoll_handler, descriptor->fd, descriptor_events(descriptor));
}

void descriptor_mask(descriptor *descriptor, enum descriptor_mask mask)
{
  if (descriptor->mask == mask)
    return;
  descriptor->mask = mask;
  reactor_modify(&descriptor->epoll_handler, descriptor->fd, descriptor_events(descriptor));
}


void descriptor_close(descriptor *descriptor)
{
  int e;

  if (!descriptor_is_open(descriptor))
    return;

  reactor_delete(&descriptor->epoll_handler, descriptor->fd);
  e = close(descriptor->fd);
  assert(e == 0);
  descriptor->fd = -1;
  descriptor->mask = 0;
}

int descriptor_fd(descriptor *descriptor)
{
  return descriptor->fd;
}

int descriptor_is_open(descriptor *descriptor)
{
  return descriptor->fd >= 0;
}
