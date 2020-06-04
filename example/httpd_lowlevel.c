#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include <dynamic.h>
#include <reactor.h>

static char *response =
  "HTTP/1.1 200 OK\r\n"
  "Date: Thu, 04 Jun 2020 20:38:15 GMT\r\n"
  "Server: libreactor\r\n"
  "Content-Length: 13\r\n"
  "Content-Type: test/plain\r\n"
  "\r\n"
  "Hello, World!";

static core_status client_event(core_event *event)
{
  stream *client = (stream *) event->state;
  void *base;
  size_t size;

  switch (event->type)
    {
    case STREAM_READ:
      stream_read(client, &base, &size);
      stream_write(client, response, strlen(response));
      stream_flush(client);
      stream_consume(client, size);
      return CORE_OK;
    case STREAM_FLUSH:
      return CORE_OK;
    default:
      stream_destruct(client);
      free(client);
      return CORE_ABORT;
    }
}

static core_status server_event(core_event *event)
{
  stream *client;
  int *s = (int *) event->state, c;

  if (event->data != EPOLLIN)
    err(1, "server");
  c = accept4(*s, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (c == -1)
    err(1, "accept4");

  client = malloc(sizeof *client);
  stream_construct(client, client_event, client);
  stream_open(client, c);
  return CORE_OK;
}

int main()
{
  int s, e;

  signal(SIGPIPE, SIG_IGN);
  core_construct(NULL);
  s = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (s == -1)
    err(1, "socket");
  (void) setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int));
  (void) setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));

  e = bind(s, (struct sockaddr *) (struct sockaddr_in[]){{.sin_family = AF_INET, .sin_port = htons(8080)}},
           sizeof(struct sockaddr_in));
  if (e == -1)
    err(1, "bind");

  (void) listen(s, INT_MAX);
  core_add(NULL, server_event, &s, s, EPOLLIN);
  core_loop(NULL);
  core_destruct(NULL);
}
