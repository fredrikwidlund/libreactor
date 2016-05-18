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

static char reply[] =
  "HTTP/1.0 200 OK\r\n"
  "Content-Length: 4\r\n"
  "Content-Type: plain/html\r\n"
  "Connection: close\r\n"
  "\r\n"
  "test";

void client_event(void *state, int type, void *data)
{
  reactor_stream *stream = state;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_ERROR:
      break;
    case REACTOR_STREAM_READ:
      reactor_stream_write_direct(stream, reply, sizeof reply - 1);
      reactor_stream_shutdown(stream);
      break;
    case REACTOR_STREAM_CLOSE:
      free(stream);
      break;
    }
}

void server_event(void *state, int type, void *data)
{
  reactor_stream *stream;
  int c, *fd, e;

  (void) state;
  if (type & REACTOR_DESC_READ)
    {
      fd = data;
      c = accept(*fd, NULL, NULL);
      if (c >= 0)
        {
          stream = malloc(sizeof *stream);
          reactor_stream_init(stream, client_event, stream);
          e = reactor_stream_open(stream, c);
          if (e == -1)
            err(1, "reactor_stream_open");
        }
    }
}

int main()
{
  reactor_desc desc;
  int s;

  reactor_core_open();

  s = socket(AF_INET, SOCK_STREAM, 0);
  assert(s != -1);
  assert(setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int)) == 0);
  assert(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) == 0);
  assert(setsockopt(s, IPPROTO_TCP, TCP_DEFER_ACCEPT, (int[]){1}, sizeof(int)) == 0);
  assert(bind(s, (struct sockaddr *) (struct sockaddr_in[])
              {{.sin_zero = 0, .sin_addr = 0, .sin_family = AF_INET, .sin_port = htons(80)}},
              sizeof(struct sockaddr_in)) == 0);
  assert(listen(s, -1) == 0);
  reactor_desc_init(&desc, server_event, &desc);
  assert(reactor_desc_open(&desc, s) == 0);
  assert(reactor_core_run() == 0);

  reactor_core_close();
}
