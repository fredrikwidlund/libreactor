#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>


struct client
{
  reactor_tcp        tcp;
  reactor_descriptor descriptor;
};

int client_event(void *state, int type, void *data)
{
  (void) state;
  (void) type;
  (void) data;
  printf("client event\n");

  return 1;
}

int tcp_event(void *state, int type, void *data)
{
  struct client *client = state;

  switch (type)
    {
    case REACTOR_TCP_EVENT_CONNECT:
      reactor_descriptor_open(&client->descriptor, client_event, client, *(int *) data, EPOLLIN | EPOLLET);
      break;
    default:
      err(1, "tcp");
    }

  return 1;
}

int main(int argc, char **argv)
{
  struct client client;

  if (argc != 3)
    err(1, "usage");

  reactor_core_construct();
  reactor_tcp_open(&client.tcp, tcp_event, &client, argv[1], argv[2], 0);
  reactor_core_run();
  reactor_core_destruct();
}
