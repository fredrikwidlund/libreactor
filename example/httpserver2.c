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

#include "reactor_core.h"

void http_event(void *state, int type, void *data)
{
  reactor_http *http = state;
  reactor_http_session *session = data;

  switch (type)
    {
    case REACTOR_HTTP_MESSAGE:
      if (strcmp(session->message.method, "GET") == 0 &&
          strcmp(session->message.path, "/") == 0)
        reactor_http_session_respond(session, (reactor_http_message[]) {{
              .version = 1, .status = 200, .reason = "OK",
              .header_size = 1, .header = (reactor_http_header[]) {{"Content-Type", "text/plain"}},
              .body_size = 4, .body = "test"
            }});
      break;
    case REACTOR_HTTP_ERROR:
      reactor_http_close(http);
      break;
    }
}

int main()
{
  reactor_http http;

  reactor_core_open();
  reactor_http_init(&http, http_event, &http);
  reactor_http_open(&http, "localhost", "80", REACTOR_HTTP_SERVER);
  assert(reactor_core_run() == 0);

  reactor_core_close();
}
