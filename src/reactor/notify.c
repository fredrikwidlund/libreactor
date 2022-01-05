#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/epoll.h>

#include "reactor.h"

static void notify_release_entry(void *arg)
{
  notify_entry *e = arg;

  free(e->path);
}

static void notify_callback(reactor_event *event)
{
  notify *notify = event->state;
  struct inotify_event *inotify_event;
  struct notify_entry *e;
  struct notify_event notify_event = {0};
  char *p, buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
  ssize_t n;
  int cancel = 0;

  notify->cancel = &cancel;
  while (1)
  {
    n = read(descriptor_fd(&notify->descriptor), buf, sizeof buf);
    if (n == -1)
    {
      assert(errno == EAGAIN);
      break;
    }
    for (p = buf; p < buf + n; p += sizeof(struct inotify_event) + inotify_event->len)
    {
      inotify_event = (struct inotify_event *) p;
      e = list_front(&notify->entries);
      while (e->id != inotify_event->wd)
      {
        e = list_next(e);
        assert(e != list_end(&notify->entries));
      }
      notify_event = (struct notify_event) {.mask = inotify_event->mask, .id = e->id,
        .name = inotify_event->len ? inotify_event->name : "", .path = e->path};
      reactor_dispatch(&notify->handler, NOTIFY_EVENT, (uintptr_t) &notify_event);
      if (cancel)
        return;
    }
  }
  notify->cancel = NULL;
}

void notify_construct(notify *notify, reactor_callback *callback, void *state)
{
  reactor_handler_construct(&notify->handler, callback, state);
  descriptor_construct(&notify->descriptor, notify_callback, notify);
  list_construct(&notify->entries);
  notify->errors = 0;
  notify->cancel = NULL;
}

void notify_clear(notify *notify)
{
  descriptor_close(&notify->descriptor);
}

int notify_watch(notify *notify, char *path, uint32_t mask)
{
  int fd, wd;
  notify_entry *entry;

  if (descriptor_fd(&notify->descriptor) == -1)
  {
    fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    assert(fd != -1);
    descriptor_open(&notify->descriptor, fd, 0);
  }

  wd = inotify_add_watch(descriptor_fd(&notify->descriptor), path, mask);
  if (wd >= 0)
  {
    entry = list_push_back(&notify->entries, NULL, sizeof *entry);
    entry->id = wd;
    entry->path = strdup(path);
  }
  return wd;
}

void notify_destruct(notify *notify)
{
  notify_clear(notify);
  list_destruct(&notify->entries, notify_release_entry);
  if (notify->cancel)
  {
    *(notify->cancel) = 1;
    notify->cancel = NULL;
  }
}
