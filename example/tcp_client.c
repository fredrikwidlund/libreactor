#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <assert.h>
#include <err.h>

#include <dynamic.h>

#include "reactor_core.h"

typedef struct client client;
struct client
{
  reactor_tcp    tcp;
  reactor_stream stream;
};

void stream_event(void *state, int type, void *data)
{
  client *client = state;
  reactor_stream_data *read = data;

  (void) data;
  printf("[stream event, type %x]\n", type);
  switch (type)
    {
    case REACTOR_STREAM_READ:
      printf("[read] %.*s\n", (int) read->size, read->base);
      read->size = 0;
      break;
    case REACTOR_STREAM_SHUTDOWN:
      reactor_stream_close(&client->stream);
      break;
    }
}

void tcp_event(void *state, int type, void *data)
{
  client *client = state;

  printf("[tcp event, type %x]\n", type);
  switch (type)
    {
    case REACTOR_TCP_CONNECT:
      reactor_stream_open(&client->stream, *(int *) data);
      /* reactor_stream_write_notify(&client->stream); */
      break;
    case REACTOR_TCP_ERROR:
      err(1, "event");
      break;
    }
}

int main(int argc, char **argv)
{
  client client;

  (void) argc;
  reactor_core_open();
  reactor_tcp_init(&client.tcp, tcp_event, &client);
  reactor_stream_init(&client.stream, stream_event, &client);
  reactor_tcp_connect(&client.tcp, argv[1], argv[2]);
  assert(reactor_core_run() == 0);
  reactor_core_close();
}
