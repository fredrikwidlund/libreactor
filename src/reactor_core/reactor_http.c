#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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
      reactor_http_session_init(session, http);
      reactor_stream_init(&session->stream, reactor_http_session_event, session);
      reactor_stream_open(&session->stream, *(int *) data);
      break;
    case REACTOR_TCP_CLOSE:
      reactor_http_close(http);
      break;
    }
}

void reactor_http_session_init(reactor_http_session *session, reactor_http *http)
{
  *session = (reactor_http_session) {.state = REACTOR_HTTP_MESSAGE_HEADER, .http = http};
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
}

void reactor_http_session_error(reactor_http_session *session)
{
  printf("http session error\n");
  reactor_http_session_close(session);
}

void reactor_http_session_read(reactor_http_session *session, reactor_stream_data *data)
{
  if (session->state == REACTOR_HTTP_MESSAGE_HEADER)
    reactor_http_session_read_header(session, data);

  if (session->state == REACTOR_HTTP_MESSAGE_BODY)
    reactor_http_session_read_body(session, data);

  if (session->state == REACTOR_HTTP_MESSAGE_COMPLETE)
    {
      session->state = REACTOR_HTTP_MESSAGE_HEADER;
      reactor_user_dispatch(&session->http->user, REACTOR_HTTP_REQUEST, session);
      /* reactor_stream_data_consume(); */
    }
  else if (session->state == REACTOR_HTTP_MESSAGE_ERROR)
    reactor_http_session_error(session);
}

void reactor_http_session_read_header(reactor_http_session *session, reactor_stream_data *data)
{
  struct phr_header fields[REACTOR_HTTP_MAX_FIELDS];
  reactor_http_header *header = &session->message.header;
  size_t method_size, path_size, i;
  int n;

  header->fields_size = REACTOR_HTTP_MAX_FIELDS;
  n = phr_parse_request(data->base, data->size,
                        (const char **) &header->method, &method_size,
                        (const char **) &header->path, &path_size,
                        &header->version,
                        fields, &header->fields_size, 0);
  if (n == -1)
    session->state = REACTOR_HTTP_MESSAGE_ERROR;
  else if (n > 0)
    {
      session->message.header_size = n;
      session->message.base = data->base;
      session->message.body_size = 0;
      header->method[method_size] = 0;
      header->path[path_size] = 0;
      for (i = 0; i < header->fields_size; i ++)
        {
          header->fields[i].name = (char *) fields[i].name;
          header->fields[i].name[fields[i].name_len] = 0;
          header->fields[i].value = (char *) fields[i].value;
          header->fields[i].value[fields[i].value_len] = 0;
          if (strcasecmp(header->fields[i].name, "Content-Length") == 0)
            session->message.body_size = strtoul(header->fields[i].value, NULL, 0);
        }
      session->state = REACTOR_HTTP_MESSAGE_BODY;
    }
}

void reactor_http_session_read_body(reactor_http_session *session, reactor_stream_data *data)
{
  if (data->size >= session->message.header_size + session->message.body_size)
    session->state = REACTOR_HTTP_MESSAGE_COMPLETE;
}

void reactor_http_session_close(reactor_http_session *session)
{
  free(session);
}
