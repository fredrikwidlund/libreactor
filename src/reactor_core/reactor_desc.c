#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

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

void reactor_desc_dispatch(reactor_desc *state, int type, void *data)
{
  reactor_user_dispatch(&((reactor_desc *) state)->user, type, data);
}
