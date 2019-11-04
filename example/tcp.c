#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

#include <dynamic.h>

#include "reactor.h"

struct tcp
{
  reactor_net    net;
  reactor_stream stream;
};

static reactor_status net_handler(reactor_event *event)
{
  struct tcp *tcp = event->state;

  switch (event->type)
    {
    case REACTOR_NET_EVENT_CONNECT:
      reactor_net_destruct(&tcp->net);
      reactor_stream_open(&tcp->stream, event->data);
      return REACTOR_ABORT;
    case REACTOR_NET_EVENT_ERROR:
    default:
      (void) fprintf(stderr, "%s\n", (char *) event->data);
      reactor_net_destruct(&tcp->net);
      return REACTOR_ABORT;
    }
}

static reactor_status stream_handler(reactor_event *event)
{
  struct tcp *tcp = event->state;

  switch (event->type)
    {
    case REACTOR_STREAM_EVENT_DATA:
      (void) printf("[data %lu]\n", reactor_stream_size(&tcp->stream));
      reactor_stream_write(&tcp->stream, reactor_stream_data(&tcp->stream), reactor_stream_size(&tcp->stream));
      reactor_stream_consume(&tcp->stream, reactor_stream_size(&tcp->stream));
      return REACTOR_OK;
    case REACTOR_STREAM_EVENT_ERROR:
    default:
      (void) fprintf(stderr, "%s\n", (char *) event->data);
      /* fall through */
    case REACTOR_STREAM_EVENT_CLOSE:
      reactor_stream_destruct(&tcp->stream);
      return REACTOR_ABORT;
    }
}

int main(int argc, char **argv)
{
  struct tcp tcp;
  int e;

  (void) reactor_construct();
  reactor_net_construct(&tcp.net, net_handler, &tcp);
  reactor_stream_construct(&tcp.stream, stream_handler, &tcp);

  e = reactor_net_connect(&tcp.net, argc >= 2 ? argv[1] : "127.0.0.1", argc >= 3 ? argv[2] : "8080");
  if (e != REACTOR_OK)
    err(1, "reactor_net_connect");

  (void) reactor_run();
  reactor_destruct();
}
