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
  reactor_http_message response;

  switch (type)
    {
    case REACTOR_HTTP_REQUEST:
      if (strcmp(session->message.header.method, "GET") == 0 &&
          strcmp(session->message.header.path, "/") == 0)
        reactor_http_message_init_response(&response, 1, 200, "OK",
                                           1, (reactor_http_field[]) {{"Content-Type", "text/plain"}},
                                           4, (void *) "test");
      else
        reactor_http_message_init_response(&response, 1, 404, "Not Found",
                                           0, NULL,
                                           0, NULL);
      reactor_http_session_respond(session, &response);
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
  reactor_http_server(&http, "localhost", "80");
  assert(reactor_core_run() == 0);

  reactor_core_close();
}
