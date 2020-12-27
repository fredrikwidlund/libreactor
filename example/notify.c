#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/inotify.h>

#include <dynamic.h>
#include <reactor.h>

static core_status callback(core_event *event)
{
  notify *notify = event->state;
  struct inotify_event *e = (struct inotify_event *) event->data;

  switch (event->type)
  {
  case NOTIFY_EVENT:
    (void) fprintf(stdout, "%04x %s\n", e->mask, e->len ? e->name : "");
    return CORE_OK;
  default:
    notify_destruct(notify);
    return CORE_ABORT;
  }
}

int main(int argc, char **argv)
{
  extern char *__progname;
  notify notify;

  if (argc != 2)
  {
    (void) fprintf(stderr, "usage: %s PATH\n", __progname);
    exit(1);
  }

  core_construct(NULL);
  notify_construct(&notify, callback, &notify);
  notify_watch(&notify, argv[1], IN_ALL_EVENTS);
  core_loop(NULL);
  core_destruct(NULL);
}
