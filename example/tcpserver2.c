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

#include "picohttpparser/picohttpparser.h"
#include "reactor_core.h"

static char reply[] =
  "HTTP/1.0 200 OK\r\n"
  "Content-Length: 4\r\n"
  "Content-Type: plain/html\r\n"
  "Connection: keep-alive\r\n"
  "\r\n"
  "test";

void client_event(void *state, int type, void *data)
{
  reactor_stream *stream = state;
  reactor_stream_data *read;
  struct phr_header fields[32];
  size_t fields_count, method_size, path_size;
  int n, minor_version;
  char *method, *path;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_ERROR:
      break;
    case REACTOR_STREAM_READ:
      read = data;
      fields_count = 32;
      n = phr_parse_request(read->base, read->size,
                            (const char **) &method, &method_size,
                            (const char **) &path, &path_size,
                            &minor_version,
                            fields, &fields_count, 0);
      if (!n)
        printf("n %d %.*s %.*s\n", n, (int) method_size, method, (int) path_size, path);

      reactor_stream_write(stream, reply, sizeof reply - 1);
      //reactor_stream_shutdown(stream);
      break;
    case REACTOR_STREAM_SHUTDOWN:
      reactor_stream_close(stream);
      break;
    case REACTOR_STREAM_CLOSE:
      free(stream);
      break;
    }
}

void server_event(void *state, int type, void *data)
{
  reactor_desc *desc = state;
  reactor_stream *stream;
  int c;

  (void) data;
  if (type & REACTOR_DESC_READ)
    {
      c = accept(reactor_desc_fd(desc), NULL, NULL);
      if (c >= 0)
        {
          stream = malloc(sizeof *stream);
          reactor_stream_init(stream, client_event, stream);
          reactor_stream_open(stream, c);
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
  reactor_desc_open(&desc, s);
  assert(reactor_core_run() == 0);

  reactor_core_close();
}
