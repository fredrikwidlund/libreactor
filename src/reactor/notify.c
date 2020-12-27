#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "reactor.h"

static core_status notify_next(core_event *event)
{
  notify *notify = event->state;

  notify->next = 0;
  return core_dispatch(&notify->user, NOTIFY_ERROR, 0);
}

static void notify_abort(notify *notify)
{
  notify->error = 1;
  notify->next = core_next(NULL, notify_next, notify);
}

static core_status notify_event(core_event *event)
{
  notify *notify = event->state;
  struct inotify_event *inotify_event;
  ssize_t n;
  char *p, buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
  core_status e;

  n = read(notify->fd, buf, sizeof buf);
  if (n == -1)
    return core_dispatch(&notify->user, NOTIFY_ERROR, 0);

  for (p = buf; p < buf + n; p += sizeof(struct inotify_event) + inotify_event->len)
  {
    inotify_event = (struct inotify_event *) p;
    e = core_dispatch(&notify->user, NOTIFY_EVENT, (uintptr_t) inotify_event);
    if (e != CORE_OK)
      return e;
  }

  return CORE_OK;
}

void notify_construct(notify *notify, core_callback *callback, void *state)
{
  *notify = (struct notify) {.user = {.callback = callback, .state = state}, .fd = -1};
}

void notify_watch(notify *notify, char *path, uint32_t mask)
{
  int fd, e;

  if (notify->fd != -1)
  {
    notify_abort(notify);
    return;
  }

  fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  if (fd == -1)
  {
    notify_abort(notify);
    return;
  }

  e = inotify_add_watch(fd, path, mask);
  if (e == -1)
  {
    (void) close(fd);
    notify_abort(notify);
    return;
  }

  notify->fd = fd;
  core_add(NULL, notify_event, notify, notify->fd, EPOLLIN);
}

void notify_destruct(notify *notify)
{
  if (notify->fd >= 0)
  {
    core_delete(NULL, notify->fd);
    (void) close(notify->fd);
    notify->fd = -1;
  }

  if (notify->next)
  {
    core_cancel(NULL, notify->next);
    notify->next = 0;
  }
}

int notify_valid(notify *notify)
{
  return notify->error == 0;
}
