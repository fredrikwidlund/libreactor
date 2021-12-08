#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "descriptor.h"

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

void descriptor_open(descriptor *descriptor, int fd, int write_notify)
{
  assert(descriptor->fd == -1);
  descriptor->fd = fd;
  descriptor->write_notify = write_notify;
  reactor_add(&descriptor->epoll_handler, descriptor->fd, EPOLLHUP | EPOLLRDHUP | EPOLLIN |
              (descriptor->write_notify ? EPOLLOUT : 0));
}

void descriptor_close(descriptor *descriptor)
{
  int e;

  if (descriptor->fd >= 0)
  {
    reactor_delete(&descriptor->epoll_handler, descriptor->fd);
    e = close(descriptor->fd);
    assert(e == 0);
    descriptor->fd = -1;
  }
}

void descriptor_write_notify(descriptor *descriptor, int write_notify)
{
  if (descriptor->write_notify != write_notify)
  {
    descriptor->write_notify = write_notify;
    reactor_modify(&descriptor->epoll_handler, descriptor->fd, EPOLLIN | EPOLLET | (descriptor->write_notify ? EPOLLOUT : 0));
  }
}

int descriptor_fd(descriptor *descriptor)
{
  return descriptor->fd;
}
