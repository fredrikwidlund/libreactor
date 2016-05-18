#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"

void reactor_desc_init(reactor_desc *desc, reactor_user_callback *callback, void *state)
{
  *desc = (reactor_desc) {.state = REACTOR_DESC_CLOSED};
  reactor_user_init(&desc->user, callback, state);
}

int reactor_desc_open(reactor_desc *desc, int fd)
{
  int e;

  if (desc->state != REACTOR_DESC_CLOSED)
    return -1;

  e = reactor_core_desc_add(desc, fd, REACTOR_DESC_READ);
  if (e == -1)
    return -1;

  desc->state = REACTOR_DESC_OPEN;
  return 0;
}

void reactor_desc_close(reactor_desc *desc)
{
  if (desc->state != REACTOR_DESC_OPEN)
    return;

  (void) close(reactor_core_desc_fd(desc));
  reactor_core_desc_remove(desc);
}

void reactor_desc_events(reactor_desc *desc, int events)
{
  if (desc->state != REACTOR_DESC_OPEN)
    return;

  reactor_core_desc_events(desc, events);
}

int reactor_desc_fd(reactor_desc *desc)
{
  if (desc->state != REACTOR_DESC_OPEN)
    return -1;

  return reactor_core_desc_fd(desc);
}

void reactor_desc_event(void *state, int type, void *data)
{
  reactor_desc *desc = state;

  if (type & POLLHUP)
    reactor_user_dispatch(&desc->user, REACTOR_DESC_CLOSE, data);
  else if (type & (POLLERR | POLLNVAL))
    reactor_user_dispatch(&desc->user, REACTOR_DESC_ERROR, data);
  else
    {
      if (type & POLLOUT)
        reactor_user_dispatch(&desc->user, REACTOR_DESC_WRITE, data);

      if (reactor_core_current() >= 0 && type & POLLIN)
        reactor_user_dispatch(&desc->user, REACTOR_DESC_READ, data);
    }
}

ssize_t reactor_desc_read(reactor_desc *desc, void *data, size_t size)
{
  return recv(reactor_core_desc_fd(desc), data, size, MSG_DONTWAIT);
}

void reactor_desc_read_notify(reactor_desc *desc, int flag)
{
  if (flag)
    reactor_core_desc_set(desc, POLLIN);
  else
    reactor_core_desc_clear(desc, POLLIN);
}

ssize_t reactor_desc_write(reactor_desc *desc, void *data, size_t size)
{
  return send(reactor_core_desc_fd(desc), data, size, MSG_DONTWAIT);
}

void reactor_desc_write_notify(reactor_desc *desc, int flag)
{
  if (flag)
    reactor_core_desc_set(desc, POLLOUT);
  else
    reactor_core_desc_clear(desc, POLLOUT);
}
