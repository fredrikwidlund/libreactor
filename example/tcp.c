#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

static const char reply[] =
  "HTTP/1.1 200 OK\r\n"
  "Date: Sun, 26 Nov 2017 18:22:42 GMT\r\n"
  "Content-Length: 3\r\n"
  "Connection: keep-alive\r\n"
  "\r\n"
  "ok\n";

struct client
{
  reactor_descriptor descriptor;
};

struct server
{
  reactor_tcp tcp;
};

int client_event(void *state, int type, void *data)
{
  struct client *client = state;
  int flags = *(int *) data;
  char request[4096];
  int fd;
  ssize_t n;

  (void) type;
  if (flags != EPOLLIN)
    {
      reactor_descriptor_close(&client->descriptor);
      free(client);
      return REACTOR_ERROR;
    }

  fd = reactor_descriptor_fd(&client->descriptor);
  n = read(fd, request, sizeof request);
  if (n <= 0)
    {
      reactor_descriptor_close(&client->descriptor);
      free(client);
      return REACTOR_ERROR;
    }

  if (!memmem(request, n, "\r\n\r\n", 4))
    err(1, "request");

  n = write(fd, reply, sizeof reply - 1);
  if (n != sizeof reply - 1)
    err(1, "write");

  return REACTOR_OK;
}

int server_event(void *state, int type, void *data)
{
  struct server *server = state;
  struct client *client;

  if (type != REACTOR_TCP_EVENT_ACCEPT)
    {
      reactor_tcp_close(&server->tcp);
      return REACTOR_ERROR;
    }

  client = malloc(sizeof *client);
  reactor_descriptor_open(&client->descriptor, client_event, client, *(int *) data, EPOLLIN | EPOLLET);
  return REACTOR_OK;
}

int main()
{
  struct server server;
  int e;

  signal(SIGPIPE, SIG_IGN);
  reactor_core_construct();
  e = reactor_tcp_open(&server.tcp, server_event, &server, "0.0.0.0", "80", REACTOR_TCP_FLAG_SERVER);
  if (e == REACTOR_ERROR)
    err(1, "reactor_tcp_open");
  reactor_core_run();
  reactor_core_destruct();
}
