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
  "Connection: keep-alive\r\n"
  "\r\n"
  "test";

void http_event(void *state, int type, void *data)
{
  reactor_http *http = state;
  reactor_http_request *request;

  (void) http;
  switch (type)
    {
    case REACTOR_HTTP_REQUEST:
      request = data;
      (void) request;
      //(void) printf("request %s %s\n", request->method, request->path);
      reactor_stream_write_direct(&request->session->stream, reply, sizeof reply - 1);
      break;
    case REACTOR_HTTP_ERROR:
      err(1, "http_event");
      break;
    }
}

int main()
{
  reactor_http http;

  reactor_core_open();
  reactor_http_init(&http, http_event, &http);
  reactor_http_server(&http, "localhost", "80");
  assert(reactor_core_run() == 0);

  reactor_core_close();
}
