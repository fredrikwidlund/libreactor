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

void reactor_http_close_final(reactor_http *http)
{
  if (http->state == REACTOR_HTTP_CLOSE_WAIT &&
      http->tcp.state == REACTOR_TCP_CLOSED &&
      http->ref == 0)
    {
      http->state = REACTOR_HTTP_CLOSED;
      reactor_user_dispatch(&http->user, REACTOR_HTTP_CLOSE, NULL);
    }
}

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

  reactor_tcp_close(&http->tcp);
  http->state = REACTOR_HTTP_CLOSE_WAIT;
  reactor_http_close_final(http);
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
      http->ref ++;
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

static inline void reactor_http_session_close_final(reactor_http_session *session)
{
  reactor_http *http;

  if (session->state == REACTOR_HTTP_SESSION_CLOSE_WAIT &&
      session->stream.state == REACTOR_STREAM_CLOSED &&
      session->ref == 0)
    {
      http = session->http;
      free(session);
      http->ref --;
      reactor_http_close_final(http);
    }
}

static inline void reactor_http_session_hold(reactor_http_session *session)
{
  session->ref ++;
}

static inline void reactor_http_session_release(reactor_http_session *session)
{
  session->ref --;
  reactor_http_session_close_final(session);
}

void reactor_http_session_init(reactor_http_session *session, reactor_http *http)
{
  *session = (reactor_http_session) {.state = REACTOR_HTTP_SESSION_HEADER, .http = http};
}

void reactor_http_session_error(reactor_http_session *session)
{
  reactor_user_dispatch(&session->http->user, REACTOR_STREAM_ERROR, NULL);
  reactor_http_session_close(session);
}

void reactor_http_session_close(reactor_http_session *session)
{
  reactor_stream_close(&session->stream);
}

void reactor_http_session_event(void *state, int type, void *data)
{
  reactor_http_session *session = state;
  reactor_stream_data *read = data;

  switch (type)
    {
    case REACTOR_STREAM_READ:
      reactor_http_session_read(session, read);
      break;
    case REACTOR_STREAM_SHUTDOWN:
      reactor_http_session_close(session);
      break;
    case REACTOR_STREAM_CLOSE:
      reactor_http_session_close_final(session);
    }
}

static inline void reactor_http_session_update_pointer(char **pointer, off_t offset)
{
  *pointer += offset;
}

static inline void reactor_http_session_update_request(reactor_http_message *message, off_t offset)
{
  size_t i;

  reactor_http_session_update_pointer((char **) &message->base, offset);
  reactor_http_session_update_pointer((char **) &message->body, offset);
  reactor_http_session_update_pointer(&message->header.method, offset);
  reactor_http_session_update_pointer(&message->header.path, offset);
  for (i = 0; i < message->header.fields_size; i ++)
    {
      reactor_http_session_update_pointer(&message->header.fields[i].name, offset);
      reactor_http_session_update_pointer(&message->header.fields[i].value, offset);
    }
}

void reactor_http_session_read(reactor_http_session *session, reactor_stream_data *data)
{
  size_t size;

  reactor_http_session_hold(session);
  while (data->size)
    {
      if (session->state == REACTOR_HTTP_SESSION_HEADER)
        reactor_http_session_read_header(session, data);
      if (session->state != REACTOR_HTTP_SESSION_BODY)
        break;

      size = session->message.header_size + session->message.body_size;
      if (size < session->message.body_size)
        {
          reactor_http_session_error(session);
          break;
        }
      if (data->size < size)
        break;

      if (session->message.base != data->base)
        reactor_http_session_update_request(&session->message, (char *) data->base - (char *) session->message.base);
      reactor_user_dispatch(&session->http->user, REACTOR_HTTP_REQUEST, session);
      if (session->state == REACTOR_HTTP_SESSION_CLOSE_WAIT)
        break;
      reactor_stream_consume(data, size);
      session->state = REACTOR_HTTP_SESSION_HEADER;
    }
  reactor_http_session_release(session);
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
      session->message.body = (char *) data->base + n;
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
      session->state = REACTOR_HTTP_SESSION_BODY;
    }
}
