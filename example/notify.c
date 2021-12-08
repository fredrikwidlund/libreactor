#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/inotify.h>

#include <reactor.h>

static void callback(reactor_event *event)
{
  notify_event *e = (notify_event *) event->data;

  (void) fprintf(stdout, "%d:%04x:%s:%s\n", e->id, e->mask, e->path, e->name);
}

int main(int argc, char **argv)
{
  extern char *__progname;
  notify notify;
  int i;
  
  if (argc < 2)
  {
    (void) fprintf(stderr, "usage: %s PATH...\n", __progname);
    exit(1);
  }

  reactor_construct();
  notify_construct(&notify, callback, NULL);
  for (i = 1; i < argc; i++)
    (void) notify_watch(&notify, argv[i], IN_ALL_EVENTS);
  reactor_loop();
  notify_destruct(&notify);
  reactor_destruct();
}
