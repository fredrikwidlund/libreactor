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

static __thread reactor_http_session *current_session = NULL;

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
  http->state = REACTOR_HTTP_INVALID;
  reactor_user_dispatch(&http->user, REACTOR_HTTP_ERROR, NULL);
}

void reactor_http_close(reactor_http *http)
{
  if (http->state != REACTOR_HTTP_OPEN &&
      http->state != REACTOR_HTTP_INVALID)
    return;

  http->state = REACTOR_HTTP_CLOSE_WAIT;
  reactor_tcp_close(&http->tcp);
}

void reactor_http_close_final(reactor_http *http)
{
  if (http->state != REACTOR_HTTP_CLOSE_WAIT ||
      http->tcp.state != REACTOR_TCP_CLOSED ||
      http->sessions)
    return;

  http->state = REACTOR_HTTP_CLOSED;
  reactor_user_dispatch(&http->user, REACTOR_HTTP_CLOSE, NULL);
}

void reactor_http_tcp_event(void *state, int type, void *data)
{
  reactor_http *http = state;
  reactor_http_session *session;
  int s;

  switch (type)
    {
    case REACTOR_TCP_ACCEPT:
      s = *(int *) data;
      session = malloc(sizeof *session);
      if (!session)
        {
          (void) close(s);
          reactor_http_error(http);
          return;
        };
      http->sessions ++;
      reactor_http_session_init(session, http);
      reactor_stream_init(&session->stream, reactor_http_session_event, session);
      reactor_stream_open(&session->stream, s);
      break;
    case REACTOR_TCP_SHUTDOWN:
      reactor_user_dispatch(&http->user, REACTOR_HTTP_SHUTDOWN, NULL);
      break;
    case REACTOR_TCP_CLOSE:
      reactor_http_close_final(http);
      break;
    case REACTOR_TCP_ERROR:
      reactor_http_error(http);
      break;
    }
}

void reactor_http_session_init(reactor_http_session *session, reactor_http *http)
{
  *session = (reactor_http_session) {.state = REACTOR_HTTP_MESSAGE_HEADER, .http = http};
}

void reactor_http_session_error(reactor_http_session *session)
{
  reactor_user_dispatch(&session->http->user, REACTOR_STREAM_ERROR, NULL);
  reactor_http_session_close(session);
}

void reactor_http_session_close(reactor_http_session *session)
{
  if (current_session == session)
    current_session = NULL;
  reactor_stream_close(&session->stream);
}

void reactor_http_session_event(void *state, int type, void *data)
{
  reactor_http_session *session = state;

  switch (type)
    {
    case REACTOR_STREAM_READ:
      reactor_http_session_read(session, (reactor_stream_data *) data);
      break;
    case REACTOR_STREAM_SHUTDOWN:
      reactor_http_session_close(session);
      break;
    case REACTOR_STREAM_CLOSE:
      session->http->sessions --;
      if (session->http->state == REACTOR_HTTP_CLOSE_WAIT)
        reactor_http_close_final(session->http);
      free(session);
    }
}

void reactor_http_session_read(reactor_http_session *session, reactor_stream_data *data)
{
  size_t size;

  current_session = session;
  while (data->size)
    {
      if (session->state == REACTOR_HTTP_MESSAGE_HEADER)
        reactor_http_session_read_header(session, data);
      if (!current_session)
        return;

      size = session->message.header_size + session->message.body_size;
      if (size < session->message.body_size)
        {
          reactor_http_session_error(session);
          return;
        }

      if (session->state != REACTOR_HTTP_MESSAGE_BODY || data->size < size)
        return;
      session->state = REACTOR_HTTP_MESSAGE_BODY;
      data->base += size;
      data->size -= size;
      reactor_user_dispatch(&session->http->user, REACTOR_HTTP_REQUEST, session);
      if (!current_session)
        return;
    }
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
    reactor_http_session_error(session);
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
