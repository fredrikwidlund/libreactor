#include <unistd.h>
#include <assert.h>
#include <sys/eventfd.h>

#include "reactor.h"

static void event_callback(reactor_event *reactor_event)
{
  event *event = reactor_event->state;
  uint64_t count;
  ssize_t n;

  assert(reactor_event->type == DESCRIPTOR_READ);
  n = read(descriptor_fd(&event->descriptor), &count, sizeof count);
  assert(n == sizeof count);
  reactor_dispatch(&event->handler, EVENT_SIGNAL, count);
}

void event_construct(event *event, reactor_callback *callback, void *state)
{
  reactor_handler_construct(&event->handler, callback, state);
  descriptor_construct(&event->descriptor, event_callback, event);
}

void event_destruct(event *event)
{
  event_close(event);
}

void event_open(event *event)
{
  int fd;

  fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
  assert(fd != -1);
  descriptor_open(&event->descriptor, fd, DESCRIPTOR_READ);
}

int event_is_open(event *event)
{
  return descriptor_is_open(&event->descriptor);
}

void event_close(event *event)
{
  if (event_is_open(event))
    descriptor_close(&event->descriptor);
}

void event_signal(event *event)
{
  ssize_t n;
  uint64_t count = 1;

  n = write(descriptor_fd(&event->descriptor), &count, sizeof count);
  assert(n == sizeof count);
}
