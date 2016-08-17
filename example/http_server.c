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

void http_event(void *state, int type, void *data)
{
  reactor_http_session *session = data;

  (void) state;
  switch (type)
    {
    case REACTOR_HTTP_MESSAGE:
      reactor_http_session_message(session, (reactor_http_message[]) {{
            .type = REACTOR_HTTP_MESSAGE_RESPONSE, .version = 1, .status = 200, .reason = "OK",
            .header_size = 1, .header = (reactor_http_header[]) {{"Content-Type", "text/plain"}},
            .body_size = 4, .body = "test"
          }});
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
