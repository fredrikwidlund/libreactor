#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include <dynamic.h>
#include "picohttpparser/picohttpparser.h"

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_stream.h"
#include "reactor_tcp.h"
#include "reactor_http.h"

void reactor_http_init(reactor_http *http, reactor_user_callback *callback, void *state)
{
  *http = (reactor_http) {.state = REACTOR_HTTP_CLOSED};
  reactor_user_init(&http->user, callback, state);
  reactor_tcp_init(&http->tcp, reactor_http_tcp_event, http);
}

void reactor_http_server(reactor_http *http, char *node, char *service)
{
  if (http->state != REACTOR_HTTP_CLOSED)
    {
      reactor_http_error(http);
      return;
    }

  http->state = REACTOR_HTTP_OPEN;
  reactor_tcp_listen(&http->tcp, node, service);
}

void reactor_http_error(reactor_http *http)
{
  printf("error\n");
  (void) http;
}

void reactor_http_close(reactor_http *http)
{
  if (http->state != REACTOR_HTTP_OPEN)
    return;

  if (http->tcp.state == REACTOR_TCP_OPEN)
    reactor_tcp_close(&http->tcp);

  if (http->tcp.state == REACTOR_TCP_CLOSED)
    {
      http->state = REACTOR_HTTP_CLOSED;
      reactor_user_dispatch(&http->user, REACTOR_HTTP_CLOSE, NULL);
    }
}

void reactor_http_tcp_event(void *state, int type, void *data)
{
  reactor_http *http = state;
  reactor_http_session *session;

  switch (type)
    {
    case REACTOR_TCP_ACCEPT:
      session = malloc(sizeof *session);
      if (!session)
        {
          reactor_http_error(http);
          return;
        };
      session->http = http;
      reactor_stream_init(&session->stream, reactor_http_session_event, session);
      reactor_stream_open(&session->stream, *(int *) data);
      break;
    case REACTOR_TCP_CLOSE:
      reactor_http_close(http);
      break;
    }
}

void reactor_http_session_event(void *state, int type, void *data)
{
  reactor_http_session *session = state;

  switch (type)
    {
    case REACTOR_STREAM_READ:
      reactor_http_session_read(session, (reactor_stream_data *) data);
      break;
    case REACTOR_STREAM_CLOSE:
      reactor_http_session_close(session);
      break;
    }

  (void) data;
}

void reactor_http_session_error(reactor_http_session *session)
{
  printf("http session error\n");
  reactor_http_session_close(session);
}

void reactor_http_session_read(reactor_http_session *session, reactor_stream_data *data)
{
  struct phr_header fields[REACTOR_HTTP_MAX_FIELDS];
  reactor_http_request request;
  size_t fields_count, method_size, path_size;
  int n;

  fields_count = REACTOR_HTTP_MAX_FIELDS;
  n = phr_parse_request(data->base, data->size,
                        (const char **) &request.method, &method_size,
                        (const char **) &request.path, &path_size,
                        &request.minor_version,
                        fields, &fields_count, 0);
  if (n == -1)
    {
      reactor_http_session_error(session);
      return;
    }

  if (n == -2)
    return;

  request.session = session;
  request.method[method_size] = 0;
  request.path[path_size] = 0;
  reactor_user_dispatch(&session->http->user, REACTOR_HTTP_REQUEST, &request);
  /* reactor_stream_data_consume(); */
}

void reactor_http_session_close(reactor_http_session *session)
{
  free(session);
}
