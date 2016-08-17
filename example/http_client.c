#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <assert.h>
#include <err.h>

#include <dynamic.h>

#include "reactor.h"

struct app
{
  char *host;
  char *service;
  char *path;
};

void http_event(void *state, int type, void *data)
{
  struct app *app = state;
  reactor_http_session *session = data;

  switch (type)
    {
    case REACTOR_HTTP_SESSION:
      reactor_http_session_message(session, (reactor_http_message[]) {{
            .type = REACTOR_HTTP_MESSAGE_REQUEST, .version = 1, .method = "GET", .path = app->path,
            .header_size = 1, .header = (reactor_http_header[]) {{"Host", app->host}},
            .body_size = 0
          }});
      reactor_stream_flush(&session->stream);
      break;
    case REACTOR_HTTP_MESSAGE:
      (void) fprintf(stderr, "[status %d]\n", session->message.status);
      (void) fprintf(stdout, "%.*s", (int) session->message.body_size, (char *) session->message.body);
      reactor_http_session_close(session);
      break;
    }
}

int main(int argc, char **argv)
{
  struct app app;
  reactor_http http;

  if (argc != 4)
    errx(1, "usage: http_client host service path");

  app = (struct app) {.host = argv[1], .service = argv[2], .path = argv[3]};
  reactor_core_open();
  reactor_http_init(&http, http_event, &app);
  reactor_http_open(&http, argv[1], argv[2], 0);
  assert(reactor_core_run() == 0);
  reactor_core_close();
}
