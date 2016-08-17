#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <err.h>

#include <dynamic.h>

#include "reactor.h"

typedef struct client client;
struct client
{
  reactor_tcp     tcp;
  reactor_stream  stream;
  char           *host;
  char           *service;
};

void stream_event(void *state, int type, void *data)
{
  client *client = state;
  reactor_stream_data *read = data;

  switch (type)
    {
    case REACTOR_STREAM_READ:
      printf("%.*s\n", (int) read->size, read->base);
      reactor_stream_consume(read, read->size);
      break;
    case REACTOR_STREAM_ERROR:
    case REACTOR_STREAM_SHUTDOWN:
      reactor_stream_close(&client->stream);
      break;
    }
}

void tcp_event(void *state, int type, void *data)
{
  client *client = state;
  char *request = "GET / HTTP/1.0\r\n\r\n";

  switch (type)
    {
    case REACTOR_TCP_CONNECT:
      reactor_stream_open(&client->stream, *(int *) data);
      reactor_stream_write(&client->stream, request, strlen(request));
      reactor_stream_flush(&client->stream);
      break;
    case REACTOR_TCP_ERROR:
      err(1, "tcp");
      break;
    }
}

int main(int argc, char **argv)
{
  client client;

  if (argc != 3)
    errx(1, "usage: tcp_client host service\n");
  client = (struct client) {.host = argv[1], .service = argv[2]};
  reactor_core_open();
  reactor_tcp_init(&client.tcp, tcp_event, &client);
  reactor_stream_init(&client.stream, stream_event, &client);
  reactor_tcp_open(&client.tcp, client.host, client.service, 0);
  assert(reactor_core_run() == 0);
  reactor_core_close();
}
