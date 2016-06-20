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

static inline void reactor_http_close_final(reactor_http *http)
{
  if (http->state == REACTOR_HTTP_CLOSE_WAIT &&
      http->tcp.state == REACTOR_TCP_CLOSED &&
      http->ref == 0)
    {
      http->state = REACTOR_HTTP_CLOSED;
      reactor_user_dispatch(&http->user, REACTOR_HTTP_CLOSE, NULL);
    }
}

static inline void reactor_http_hold(reactor_http *http)
{
  http->ref ++;
}

static inline void reactor_http_release(reactor_http *http)
{
  http->ref --;
  reactor_http_close_final(http);
}

void reactor_http_init(reactor_http *http, reactor_user_callback *callback, void *state)
{
  *http = (reactor_http) {.state = REACTOR_HTTP_CLOSED};
  reactor_user_init(&http->user, callback, state);
  reactor_tcp_init(&http->tcp, reactor_http_event, http);
}

void reactor_http_open(reactor_http *http, char *node, char *service, int flags)
{
  if (http->state != REACTOR_HTTP_CLOSED)
    {
      reactor_http_error(http);
      return;
    }

  http->flags = flags;
  http->state = REACTOR_HTTP_OPEN;
  reactor_tcp_open(&http->tcp, node, service, flags & REACTOR_HTTP_SERVER ? REACTOR_TCP_SERVER : 0);
}

void reactor_http_error(reactor_http *http)
{
  http->state = REACTOR_HTTP_INVALID;
  reactor_user_dispatch(&http->user, REACTOR_HTTP_ERROR, NULL);
}

void reactor_http_close(reactor_http *http)
{
  if (http->state == REACTOR_HTTP_CLOSED)
    return;

  reactor_tcp_close(&http->tcp);
  http->state = REACTOR_HTTP_CLOSE_WAIT;
  reactor_http_close_final(http);
}

void reactor_http_event(void *state, int type, void *data)
{
  reactor_http *http = state;
  reactor_http_session *session;
  int s = *(int *) data;

  switch (type)
    {
    case REACTOR_TCP_CONNECT:
    case REACTOR_TCP_ACCEPT:
      reactor_http_session_new(&session, http);
      if (!session)
        {
          (void) close(s);
          reactor_http_error(http);
          return;
        };
      reactor_http_hold(http);
      reactor_stream_init(&session->stream, reactor_http_session_event, session);
      reactor_stream_open(&session->stream, s);
      if (http->state == REACTOR_HTTP_OPEN)
        reactor_user_dispatch(&http->user, REACTOR_HTTP_SESSION, session);
      reactor_http_release(http);
      break;
    case REACTOR_TCP_SHUTDOWN:
      reactor_user_dispatch(&http->user, REACTOR_HTTP_SHUTDOWN, NULL);
      break;
    case REACTOR_TCP_ERROR:
      reactor_http_error(http);
      break;
    case REACTOR_TCP_CLOSE:
      reactor_http_close_final(http);
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

static inline void reactor_http_session_update_reference(char **reference, off_t offset)
{
  *reference += offset;
}

static inline void reactor_http_session_update(reactor_http_session *session, off_t offset)
{
  reactor_http_message *message = &session->message;
  size_t i;

  reactor_http_session_update_reference((char **) &session->message_base, offset);
  if (message->type == REACTOR_HTTP_MESSAGE_REQUEST)
    {
      reactor_http_session_update_reference(&message->method, offset);
      reactor_http_session_update_reference(&message->path, offset);
    }
  else
    {
      reactor_http_session_update_reference(&message->reason, offset);
    }
  for (i = 0; i < message->header_size; i ++)
    {
      reactor_http_session_update_reference(&message->header[i].name, offset);
      reactor_http_session_update_reference(&message->header[i].value, offset);
    }
}

static inline void reactor_http_session_parse(reactor_http_session *session, reactor_stream_data *data)
{
  struct phr_header header[REACTOR_HTTP_MAX_HEADERS];
  reactor_http_message *message = &session->message;
  size_t method_size = 0, reason_size = 0, path_size = 0, i;
  int n;

  message->header_size = REACTOR_HTTP_MAX_HEADERS;
  if (message->type == REACTOR_HTTP_MESSAGE_REQUEST)
    n = phr_parse_request(data->base, data->size,
                          (const char **) &message->method, &method_size,
                          (const char **) &message->path, &path_size,
                          &message->version,
                          header, &message->header_size, 0);
  else
    n = phr_parse_response(data->base, data->size,
                           &message->version,
                           &message->status,
                           (const char **) &message->reason, &reason_size,
                           header, &message->header_size, 0);
  if (n == -1)
    reactor_http_session_error(session);
  if (n <= 0)
    return;

  session->message_base = data->base;
  session->body_offset = n;
  message->body_size = 0;
  message->header = session->header_storage;
  for (i = 0; i < message->header_size; i ++)
    {
      message->header[i].name = (char *) header[i].name;
      message->header[i].name[header[i].name_len] = 0;
      message->header[i].value = (char *) header[i].value;
      message->header[i].value[header[i].value_len] = 0;
      if (strcasecmp(message->header[i].name, "Content-Length") == 0)
        message->body_size = strtoul(message->header[i].value, NULL, 0);
    }
  if (session->body_offset + message->body_size < session->body_offset)
    reactor_http_session_error(session);
  if (message->type == REACTOR_HTTP_MESSAGE_REQUEST)
    {
      message->method[method_size] = 0;
      message->path[path_size] = 0;
    }
  else
    {
      message->reason[reason_size] = 0;
    }
}

static inline void reactor_http_session_read(reactor_http_session *session, reactor_stream_data *data)
{
  size_t size;

  reactor_http_session_hold(session);
  while (data->size)
    {
      if (!session->message_base)
        {
          reactor_http_session_parse(session, data);
          if (!session->message_base || session->state != REACTOR_HTTP_SESSION_OPEN)
            break;
        }
      size = session->body_offset + session->message.body_size;
      if (data->size < size)
        break;
      session->message.body = (char *) data->base + session->body_offset;
      if (data->base != session->message_base)
        reactor_http_session_update(session, (char *) data->base - (char *) session->message_base);
      reactor_user_dispatch(&session->http->user, REACTOR_HTTP_MESSAGE, session);
      if (session->state != REACTOR_HTTP_SESSION_OPEN)
        break;
      reactor_stream_consume(data, size);
      session->message_base = NULL;
    }
  reactor_http_session_release(session);
}

void reactor_http_session_new(reactor_http_session **session, reactor_http *http)
{
  *session = malloc(sizeof **session);
  if (!*session)
    return;
  **session = (reactor_http_session)
    {
      .state = REACTOR_HTTP_SESSION_OPEN,
      .http = http,
      .message.type = http->flags & REACTOR_HTTP_SERVER ? REACTOR_HTTP_MESSAGE_REQUEST : REACTOR_HTTP_MESSAGE_RESPONSE,
    };
  http->ref ++;
}

void reactor_http_session_error(reactor_http_session *session)
{
  reactor_user_dispatch(&session->http->user, REACTOR_STREAM_ERROR, NULL);
  reactor_http_session_close(session);
}

void reactor_http_session_close(reactor_http_session *session)
{
  if (session->state == REACTOR_HTTP_SESSION_CLOSED)
    return;

  reactor_stream_close(&session->stream);
  session->state = REACTOR_HTTP_SESSION_CLOSE_WAIT;
  reactor_http_session_close_final(session);
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
    case REACTOR_STREAM_ERROR:
      reactor_http_session_error(session);
      break;
    case REACTOR_STREAM_SHUTDOWN:
      reactor_http_session_close(session);
      break;
    case REACTOR_STREAM_CLOSE:
      reactor_http_session_close_final(session);
    }
}

void reactor_http_session_message(reactor_http_session *session, reactor_http_message *message)
{
  reactor_stream *stream = &session->stream;
  size_t i;

  if (message->type == REACTOR_HTTP_MESSAGE_REQUEST)
    {
      reactor_stream_write_string(stream, message->method);
      reactor_stream_write_string(stream, " ");
      reactor_stream_write_string(stream, message->path);
      reactor_stream_write_string(stream, " ");
      reactor_stream_write_string(stream, message->version ? "HTTP/1.1\r\n" : "HTTP/1.0\r\n");
    }
  else
    {
      reactor_stream_write_string(stream, message->version ? "HTTP/1.1 " : "HTTP/1.0 ");
      reactor_stream_write_unsigned(stream, message->status);
      reactor_stream_write_string(stream, " ");
      reactor_stream_write_string(stream, message->reason);
      reactor_stream_write_string(stream, "\r\n");
    }
  for (i = 0; i < message->header_size; i ++)
    {
      reactor_stream_write_string(stream, message->header[i].name);
      reactor_stream_write_string(stream, ": ");
      reactor_stream_write_string(stream, message->header[i].value);
      reactor_stream_write_string(stream, "\r\n");
    }
  if (message->type == REACTOR_HTTP_MESSAGE_RESPONSE || message->body_size)
    {
      reactor_stream_write_string(stream, "Content-Length: ");
      reactor_stream_write_unsigned(stream, message->body_size);
      reactor_stream_write_string(stream, "\r\n");
    }
  reactor_stream_write_string(stream, "\r\n");
  reactor_stream_write(stream, message->body, message->body_size);
}
