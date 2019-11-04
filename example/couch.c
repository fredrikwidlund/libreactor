#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>

#include <dynamic.h>
#include <jansson.h>

#include "reactor.h"

struct client
{
  reactor_couch couch;
};

static reactor_status client_handler(reactor_event *event)
{
  struct client *client = event->state;
  json_t *v;
  char *s;

  switch (event->type)
    {
    case REACTOR_COUCH_EVENT_STATUS:
      switch (event->data)
        {
        case REACTOR_COUCH_STATUS_ONLINE:
          (void) fprintf(stderr, "[online]\n");
          break;
        case REACTOR_COUCH_STATUS_STALE:
          (void) fprintf(stderr, "[stale]\n");
          break;
        }
      return REACTOR_OK;
    case REACTOR_COUCH_EVENT_WARNING:
      (void) fprintf(stderr, "[warning] %s\n", (char *) event->data);
      return REACTOR_OK;
    case REACTOR_COUCH_EVENT_ERROR:
      (void) fprintf(stderr, "[error] %s\n", (char *) event->data);
      reactor_couch_destruct(&client->couch);
      return REACTOR_ABORT;
    case REACTOR_COUCH_EVENT_UPDATE:
      v = (json_t *) event->data;
      s = json_dumps(v, 0);
      printf("[value] %s\n", s);
      free(s);
      return REACTOR_OK;
    default:
      (void) fprintf(stderr, "[couch] event type %d\n", event->type);
      return REACTOR_OK;
    }
}

int main(int argc, char **argv)
{
  struct client client = {0};

  (void) reactor_construct();
  (void) reactor_couch_construct(&client.couch, client_handler, &client);
  reactor_couch_open(&client.couch, argc == 2 ? argv[1] : "http://127.0.0.1:5984/test");
  (void) reactor_run();
  reactor_destruct();
}
