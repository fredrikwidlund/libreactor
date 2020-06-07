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

static void plaintext(stream *client)
{
  http_response response;
  http_response_ok(&response, segment_string("text/plain"), segment_string("Hello, World!"));
  http_response_write(&response, stream_allocate(client, http_response_size(&response)));
}

static void not_found(stream *client)
{
  http_response response;
  http_response_not_found(&response);
  http_response_write(&response, stream_allocate(client, http_response_size(&response)));
}

static core_status client_event(core_event *event)
{
  stream *client = (stream *) event->state;
  http_request request;
  segment s;
  size_t offset = 0;
  ssize_t n;

  switch (event->type)
    {
    case STREAM_READ:
      s = stream_read(client);
      do
        {
          n = http_request_read(&request, segment_offset(s, offset));
          if (n <= 0)
            break;
          if (segment_equal(request.method, segment_string("GET")) && segment_equal(request.target, segment_string("/plaintext")))
            plaintext(client);
          else
            not_found(client);
          offset += n;
        }
      while (offset < s.size);

      if (n == -1)
        {
          stream_destruct(client);
          free(client);
          return CORE_ABORT;
       }

      stream_flush(client);
      stream_consume(client, offset);
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
