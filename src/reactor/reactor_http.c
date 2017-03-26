#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_pool.h"
#include "reactor_core.h"
#include "reactor_resolver.h"
#include "reactor_stream.h"
#include "reactor_tcp.h"
#include "reactor_http_parser.h"
#include "reactor_http.h"

static void reactor_http_error(reactor_http *http)
{
  reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_ERROR, NULL);
}

static void reactor_http_client_event(void *state, int type, void *data)
{
  reactor_http *http = state;
  reactor_http_header headers[REACTOR_HTTP_HEADERS_MAX];
  reactor_http_response response;
  int e;

  switch (type)
    {
    case REACTOR_STREAM_EVENT_READ:
      while (reactor_stream_data_size(data))
        {
          response.header_count = REACTOR_HTTP_HEADERS_MAX;
          response.headers = headers;
          e = reactor_http_parser_read_response(&http->parser, &response, data);
          if (e == -1)
            {
              reactor_http_error(http);
              break;
            }

          if (e == 0)
            break;

          reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_RESPONSE, &response);
        }
      break;
    case REACTOR_STREAM_EVENT_ERROR:
      reactor_http_error(http);
      break;
    case REACTOR_STREAM_EVENT_HANGUP:
      reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_HANGUP, NULL);
      break;
    case REACTOR_STREAM_EVENT_CLOSE:
      reactor_http_release(http);
    }
}

static void reactor_http_server_event(void *state, int type, void *data)
{
  reactor_http *http = state;
  reactor_http_header headers[REACTOR_HTTP_HEADERS_MAX];
  reactor_http_request request;
  int e;

  switch (type)
    {
    case REACTOR_STREAM_EVENT_READ:
      while (reactor_stream_data_size(data))
        {
          request.header_count = REACTOR_HTTP_HEADERS_MAX;
          request.headers = headers;
          e = reactor_http_parser_read_request(&http->parser, &request, data);
          if (e == -1)
            {
              reactor_http_error(http);
              break;
            }

          if (e == 0)
            break;

          reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_REQUEST, &request);
        }
      reactor_http_flush(http);
      break;
    case REACTOR_STREAM_EVENT_ERROR:
      reactor_http_error(http);
      break;
    case REACTOR_STREAM_EVENT_HANGUP:
      reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_HANGUP, NULL);
      break;
    case REACTOR_STREAM_EVENT_CLOSE:
      reactor_http_release( http);
    }
}

void reactor_http_hold(reactor_http *http)
{
  http->ref ++;
}

void reactor_http_release(reactor_http *http)
{
  http->ref --;
  if (!http->ref)
    {
      http->state = REACTOR_HTTP_STATE_CLOSED;
      reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_CLOSE, NULL);
    }
}

void reactor_http_open(reactor_http *http, reactor_user_callback *callback, void *state, int socket, int flags)
{
  http->ref = 0;
  http->state = REACTOR_HTTP_STATE_OPEN;
  http->flags = flags;
  reactor_user_construct(&http->user, callback, state);
  reactor_http_parser_construct(&http->parser);
  reactor_http_hold(http);
  reactor_stream_open(&http->stream, flags & REACTOR_HTTP_FLAG_SERVER ?
                      reactor_http_server_event : reactor_http_client_event, http, socket);
}

void reactor_http_close(reactor_http *http)
{
  if (http->state & (REACTOR_HTTP_STATE_CLOSED | REACTOR_HTTP_STATE_CLOSING))
    return;

  http->state = REACTOR_HTTP_STATE_CLOSING;
  reactor_stream_close(&http->stream);
}

void reactor_http_write_request(reactor_http *http, reactor_http_request *request)
{
  reactor_http_write_request_line(http, request->method, request->path, request->version);
  reactor_http_write_headers(http, request->headers, request->header_count);
  if (request->size)
    reactor_http_write_content_length(http, request->size);
  reactor_http_write_end(http);
  if (request->size)
    reactor_http_write_body(http, request->data, request->size);
}

void reactor_http_write_response(reactor_http *http, reactor_http_response *response)
{
  reactor_http_write_status_line(http, response->version, response->status, response->reason);
  reactor_http_write_headers(http, response->headers, response->header_count);
  reactor_http_write_content_length(http, response->size);
  reactor_http_write_end(http);
  if (response->size)
    reactor_http_write_body(http, response->data, response->size);
}

void reactor_http_write_request_line(reactor_http *http, char *method, char *path, int version)
{
  reactor_stream_write(&http->stream, method, strlen(method));
  reactor_stream_write(&http->stream, " ", 1);
  reactor_stream_write(&http->stream, path, strlen(path));
  reactor_stream_write(&http->stream, " ", 1);
  reactor_stream_write(&http->stream, version ? "HTTP/1.1\r\n" : "HTTP/1.0\r\n", 10);
}

void reactor_http_write_status_line(reactor_http *http, int version, int status, char *reason)
{
  reactor_stream_write(&http->stream, version ? "HTTP/1.1 " : "HTTP/1.0 ", 9);
  reactor_stream_write_unsigned(&http->stream, status);
  reactor_stream_write(&http->stream, " ", 1);
  reactor_stream_write(&http->stream, reason, strlen(reason));
  reactor_stream_write(&http->stream, "\r\n", 2);
}

void reactor_http_write_headers(reactor_http *http, reactor_http_header *headers, size_t count)
{
  size_t i;

  for (i = 0; i < count; i ++)
    {
      reactor_stream_write(&http->stream, headers[i].name, strlen(headers[i].name));
      reactor_stream_write(&http->stream, ": ", 2);
      reactor_stream_write(&http->stream, headers[i].value, strlen(headers[i].value));
      reactor_stream_write(&http->stream, "\r\n", 2);
    }
}

void reactor_http_write_content_length(reactor_http *http, size_t size)
{
  reactor_stream_write(&http->stream, "Content-Length: ", 16);
  reactor_stream_write_unsigned(&http->stream, size);
  reactor_stream_write(&http->stream, "\r\n", 2);
}

void reactor_http_write_end(reactor_http *http)
{
  reactor_stream_write(&http->stream, "\r\n", 2);
}

void reactor_http_write_body(reactor_http *http, void *base, size_t size)
{
  reactor_stream_write(&http->stream, base, size);
}

void reactor_http_flush(reactor_http *http)
{
  reactor_stream_flush(&http->stream);
}
