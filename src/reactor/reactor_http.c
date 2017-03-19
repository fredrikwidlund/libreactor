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
#include "reactor_http.h"

static void reactor_http_error(reactor_http *http)
{
  reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_ERROR, NULL);
}

static void reactor_http_client_event(void *state, int type, void *data)
{
  reactor_http *http = state;
  reactor_stream_data *read;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_EVENT_READ:
      read = data;
      printf("read %.*s\n", (int) read->size, (char *) read->base);
      /*
      while (reactor_stream_data_size(data))
        {
          response.headers = (reactor_http_headers) {.count = REACTOR_HTTP_HEADERS_MAX, .header = header};
          token = reactor_http_parser_response(&http->parser, &response, data);
          if (reactor_likely(token == REACTOR_HTTP_PARSER_MESSAGE))
            reactor_user_dispatch(&http->user, REACTOR_HTTP_RESPONSE, &response);
          else
            {
              if (token == REACTOR_HTTP_PARSER_INVALID)
                {
                  reactor_stream_close(&http->stream);
                  reactor_user_dispatch(&http->user, REACTOR_HTTP_ERROR, http);
                }
              break;
            }
        }
      */
      break;
    case REACTOR_STREAM_EVENT_ERROR:
      reactor_http_error(http);
      break;
    case REACTOR_STREAM_EVENT_HANGUP:
      break;
    case REACTOR_STREAM_EVENT_CLOSE:
      reactor_http_release(http);
    }

}

static void reactor_http_server_event(void *state, int type, void *data)
{
  reactor_http *http = state;

  (void) http;
  (void) type;
  (void) data;
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
  reactor_http_hold(http);
  reactor_stream_open(&http->stream, flags & REACTOR_HTTP_FLAG_SERVER ?
                      reactor_http_server_event : reactor_http_client_event, http, socket);
}

void reactor_http_close(reactor_http *http)
{
  (void) http;
}

void reactor_http_write_request(reactor_http *http, reactor_http_request *request)
{
  reactor_http_write_request_line(http, request->method, request->path, request->version);
  reactor_http_write_headers(http, request->headers);
  reactor_http_write_end(http);
  reactor_http_write_body(http, request->data, request->size);
}

void reactor_http_write_request_line(reactor_http *http, char *method, char *path, int version)
{
  reactor_stream_write(&http->stream, method, strlen(method));
  reactor_stream_write(&http->stream, " ", 1);
  reactor_stream_write(&http->stream, path, strlen(path));
  reactor_stream_write(&http->stream, " ", 1);
  reactor_stream_write(&http->stream, version ? "HTTP/1.1\r\n" : "HTTP/1.0\r\n", 10);
}

void reactor_http_write_headers(reactor_http *http, reactor_http_header *headers)
{
  size_t i;

  for (i = 0; headers[i].name; i ++)
    {
      reactor_stream_write(&http->stream, headers[i].name, strlen(headers[i].name));
      reactor_stream_write(&http->stream, ": ", 2);
      reactor_stream_write(&http->stream, headers[i].value, strlen(headers[i].value));
      reactor_stream_write(&http->stream, "\r\n", 2);
    }
}

void reactor_http_write_body(reactor_http *http, void *base, size_t size)
{
  (void) http;
  (void) base;
  (void) size;
}

void reactor_http_write_end(reactor_http *http)
{
  reactor_stream_write(&http->stream, "\r\n", 2);
}

void reactor_http_flush(reactor_http *http)
{
  reactor_stream_flush(&http->stream);
}
