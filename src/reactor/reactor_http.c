#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>
#include <netdb.h>

#include <dynamic.h>

#include "picohttpparser/picohttpparser.h"
#include "reactor_util.h"
#include "reactor_user.h"
#include "reactor_core.h"
#include "reactor_descriptor.h"
#include "reactor_stream.h"
#include "reactor_http.h"

/* reactor_http_error */

static int reactor_http_error(reactor_http *http)
{
  return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_ERROR, NULL);
}

/* reactor_http_parse */

static ssize_t reactor_http_parse_chunk(struct iovec *data, struct iovec *chunk)
{
  char *p;

  p = memchr(data->iov_base, '\n', data->iov_len);
  if (!p)
    return 0;
  p ++;
  chunk->iov_base = p;
  chunk->iov_len = strtoull(data->iov_base, NULL, 16);
  if (chunk->iov_len >= REACTOR_HTTP_MAX_CHUNK_SIZE)
    return -1;
  p += chunk->iov_len + 2;
  if (p > (char *) data->iov_base + data->iov_len)
    return 0;
  if (strncmp(p - 2, "\r\n", 2) != 0)
    return -1;
  return p - (char *) data->iov_base;
}

static ssize_t reactor_http_parse_chunks(struct iovec *data, struct iovec *content)
{
  ssize_t n;
  struct iovec segment = *data, chunk;

  if (content)
    *content = (struct iovec) {segment.iov_base, 0};

  while (segment.iov_len)
    {
      n = reactor_http_parse_chunk(&segment, &chunk);
      if (n <= 0)
        return n == 0 ? 0 : -1;
      segment.iov_base = (char *) segment.iov_base + n;
      segment.iov_len -= n;
      if (!chunk.iov_len)
        return (char *) segment.iov_base - (char *) data->iov_base;
      if (content)
        {
          memmove((char *) content->iov_base + content->iov_len, chunk.iov_base, chunk.iov_len);
          content->iov_len += chunk.iov_len;
        }
    }

  return 0;
}

static ssize_t reactor_http_parse_response_headers(struct iovec *data, reactor_http_response *response)
{
  response->headers = REACTOR_HTTP_MAX_HEADERS;
  response->body = (struct iovec) {0};
  return phr_parse_response(data->iov_base, data->iov_len,
                            &response->version, &response->status,
                            (const char **) &response->reason.iov_base, &response->reason.iov_len,
                            (struct phr_header *) response->header, &response->headers, 0);
}

static ssize_t reactor_http_parse_request_headers(struct iovec *data, reactor_http_request *request)
{
  request->headers = REACTOR_HTTP_MAX_HEADERS;
  request->body = (struct iovec) {0};
  return phr_parse_request(data->iov_base, data->iov_len,
                           (const char **) &request->method.iov_base, &request->method.iov_len,
                           (const char **) &request->path.iov_base, &request->path.iov_len,
                           &request->version,
                           (struct phr_header *) request->header, &request->headers, 0);
}

static ssize_t reactor_http_parse_response(struct iovec *data, reactor_http_response *response)
{
  ssize_t n;
  size_t header_size;
  struct iovec *value, segment;

  n = reactor_http_parse_response_headers(data, response);
  if (reactor_unlikely(n <= 0))
    return n == -2 ? 0 : -1;
  header_size = n;

  value = reactor_http_header_get(response->header, response->headers, "Content-Length");
  if (reactor_unlikely(value))
    {
      response->body.iov_base = (char *) data->iov_base + header_size;
      response->body.iov_len = strtoull(value->iov_base, NULL, 10);
      if (response->body.iov_len > data->iov_len - header_size)
        return 0;
      return header_size + response->body.iov_len;
    }

  value = reactor_http_header_get(response->header, response->headers, "Transfer-Encoding");
  if (reactor_http_header_value(value, "chunked"))
    {
      segment.iov_base = (char *) data->iov_base + header_size;
      segment.iov_len = data->iov_len - header_size;
      n = reactor_http_parse_chunks(&segment, NULL);
      if (n <= 0)
        return n == 0 ? 0 : -1;
      n = reactor_http_parse_chunks(&segment, &response->body);
      return header_size + n;
    }

  return -1;
}

static ssize_t reactor_http_parse_request(struct iovec *data, reactor_http_request *request)
{
  ssize_t n;
  size_t header_size;
  struct iovec *value, segment;

  n = reactor_http_parse_request_headers(data, request);
  if (reactor_unlikely(n <= 0))
    return n == -2 ? 0 : -1;
  header_size = n;

  value = reactor_http_header_get(request->header, request->headers, "Content-Length");
  if (reactor_unlikely(value))
    {
      request->body.iov_base = (char *) data->iov_base + header_size;
      request->body.iov_len = strtoull(value->iov_base, NULL, 10);
      if (request->body.iov_len > data->iov_len - header_size)
        return 0;
      return header_size + request->body.iov_len;
    }

  value = reactor_http_header_get(request->header, request->headers, "Transfer-Encoding");
  if (reactor_unlikely(value))
    {
      if (!reactor_http_header_value(value, "chunked"))
        return -1;

      segment.iov_base = (char *) data->iov_base + header_size;
      segment.iov_len = data->iov_len - header_size;
      n = reactor_http_parse_chunks(&segment, NULL);
      if (n <= 0)
        return n == 0 ? 0 : -1;
      n = reactor_http_parse_chunks(&segment, &request->body);
      return header_size + n;
    }

  return header_size;
}

/* reactor_http_request */

static size_t reactor_http_request_size(reactor_http_request *request)
{
  size_t size, i;

  size = 14 + request->method.iov_len + request->path.iov_len + (request->headers * 4) + request->body.iov_len;
  for (i = 0; i < request->headers; i ++)
    size += request->header[i].name.iov_len + request->header[i].value.iov_len;
  return size;
}

static void reactor_http_request_write(reactor_http_request *request, void *base)
{
  char *p = base;
  size_t i;

  memcpy(p, request->method.iov_base, request->method.iov_len);
  p += request->method.iov_len;
  *p ++ = ' ';
  memcpy(p, request->path.iov_base, request->path.iov_len);
  p += request->path.iov_len;
  memcpy(p, " HTTP/1.0\r\n", 11);
  p[8] = request->version + '0';
  p += 11;
  for (i = 0; i < request->headers; i ++)
    {
      memcpy(p, request->header[i].name.iov_base, request->header[i].name.iov_len);
      p += request->header[i].name.iov_len;
      *p ++ = ':';
      *p ++ = ' ';
      memcpy(p, request->header[i].value.iov_base, request->header[i].value.iov_len);
      p += request->header[i].value.iov_len;
      *p ++ = '\r';
      *p ++ = '\n';
    }
  *p ++ = '\r';
  *p ++ = '\n';
  memcpy(p, request->body.iov_base, request->body.iov_len);
}

/* reactor_http_response */

static size_t reactor_http_response_size(reactor_http_response *response)
{
  size_t size, i;

  size = 17 + response->reason.iov_len + (response->headers * 4) + response->body.iov_len;
  for (i = 0; i < response->headers; i ++)
    size += response->header[i].name.iov_len + response->header[i].value.iov_len;
  return size;
}

static void reactor_http_response_write(reactor_http_response *response, void *base)
{
  char *p = base;
  size_t i;

  memcpy(p, "HTTP/1.0 500 ", 13);
  p[7] = response->version + '0';
  p += 12;
  reactor_util_u32sprint(response->status, p);
  p ++;
  memcpy(p, response->reason.iov_base, response->reason.iov_len);
  p += response->reason.iov_len;
  *p ++ = '\r';
  *p ++ = '\n';
  for (i = 0; i < response->headers; i ++)
    {
      memcpy(p, response->header[i].name.iov_base, response->header[i].name.iov_len);
      p += response->header[i].name.iov_len;
      *p ++ = ':';
      *p ++ = ' ';
      memcpy(p, response->header[i].value.iov_base, response->header[i].value.iov_len);
      p += response->header[i].value.iov_len;
      *p ++ = '\r';
      *p ++ = '\n';
    }
  *p ++ = '\r';
  *p ++ = '\n';
  memcpy(p, response->body.iov_base, response->body.iov_len);
}

/* reactor_http_server */

static int reactor_http_server_stream(reactor_http *http, struct iovec *data)
{
  reactor_http_request request;
  ssize_t n;
  int e;
  struct iovec *value, chunk;

  while (1)
    {
      switch (http->state)
        {
        case REACTOR_HTTP_STATE_HEADER:
          n = reactor_http_parse_request_headers(data, &request);
          if (reactor_unlikely(n <= 0))
            return n == -2 ? REACTOR_OK : reactor_http_error(http);
          data->iov_base = (char *) data->iov_base + n;
          data->iov_len -= n;

          e = reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_REQUEST_HEADER, &request);
          if (e == REACTOR_ABORT)
            return REACTOR_ABORT;

          http->remaining = 0;
          value = reactor_http_header_get(request.header, request.headers, "Content-Length");
          if (value)
            http->remaining = strtoul(value->iov_base, NULL, 0);

          value = reactor_http_header_get(request.header, request.headers, "Transfer-Encoding");
          if (reactor_unlikely(value))
            {
              if (!reactor_http_header_value(value, "chunked"))
                return reactor_http_error(http);
              http->state = REACTOR_HTTP_STATE_CHUNK;
              break;
            }

          http->state = REACTOR_HTTP_STATE_DATA;
          /* fall through */
        case REACTOR_HTTP_STATE_DATA:
          n = MIN(data->iov_len, http->remaining);
          if (n)
            {
              e = reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_REQUEST_DATA,
                                        (struct iovec []){{.iov_base = data->iov_base, .iov_len = n}});
              if (e != REACTOR_OK)
                return REACTOR_ABORT;
              data->iov_base = (char *) data->iov_base + n;
              data->iov_len -= n;
              http->remaining -= n;
            }

          if (!http->remaining)
            {
              e = reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_REQUEST_END, NULL);
              if (e != REACTOR_OK)
                return REACTOR_ABORT;
              http->state = REACTOR_HTTP_STATE_HEADER;
            }

          if (!data->iov_len)
            return reactor_http_flush(http);
          break;
        case REACTOR_HTTP_STATE_CHUNK:
          n = reactor_http_parse_chunk(data, &chunk);
          if (reactor_unlikely(n <= 0))
            return n == 0 ? REACTOR_OK : reactor_http_error(http);

          if (chunk.iov_len)
            e = reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_REQUEST_DATA, &chunk);
          else
            {
              http->state = REACTOR_HTTP_STATE_HEADER;
              e = reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_REQUEST_END, NULL);
            }
          if (e != REACTOR_OK)
            return REACTOR_ABORT;

          data->iov_base = (char *) data->iov_base + n;
          data->iov_len -= n;
          if (!data->iov_len)
            return reactor_http_flush(http);
          break;
        default:
          return reactor_http_error(http);
        }
    }
}

static int reactor_http_server_read(reactor_http *http, struct iovec *data)
{
  reactor_http_request request;
  ssize_t n;
  int e;

  while (data->iov_len)
    {
      n = reactor_http_parse_request(data, &request);
      if (reactor_unlikely(n <= 0))
        {
          if (n == 0)
            break;
          return reactor_http_error(http);
        }
      e = reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_REQUEST, &request);
      if (e != REACTOR_OK)
        return REACTOR_ABORT;
      data->iov_base = (char *) data->iov_base + n;
      data->iov_len -= n;
    }

  return reactor_http_flush(http);
}

static int reactor_http_server_stream_event(void *state, int type, void *data)
{
  reactor_http *http = state;

  if (reactor_likely(type == REACTOR_STREAM_EVENT_READ))
    return reactor_http_server_stream(http, data);
  switch (type)
    {
    case REACTOR_STREAM_EVENT_CLOSE:
      return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_CLOSE, NULL);
    case REACTOR_STREAM_EVENT_WRITE:
      return REACTOR_OK;
    default:
      return reactor_http_error(http);
    }
}

static int reactor_http_server_event(void *state, int type, void *data)
{
  reactor_http *http = state;

  if (reactor_likely(type == REACTOR_STREAM_EVENT_READ))
    return reactor_http_server_read(http, data);
  switch (type)
    {
    case REACTOR_STREAM_EVENT_CLOSE:
      return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_CLOSE, NULL);
    case REACTOR_STREAM_EVENT_WRITE:
      return REACTOR_OK;
    default:
      return reactor_http_error(http);
    }
}

/* reactor_http_client */

static int reactor_http_client_stream(reactor_http *http, struct iovec *data)
{
  reactor_http_response response;
  ssize_t n;
  int e;
  struct iovec *value, chunk;

  while (1)
    {
      switch (http->state)
        {
        case REACTOR_HTTP_STATE_HEADER:
          n = reactor_http_parse_response_headers(data, &response);
          if (reactor_unlikely(n <= 0))
            return n == -2 ? REACTOR_OK : reactor_http_error(http);
          data->iov_base = (char *) data->iov_base + n;
          data->iov_len -= n;

          e = reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_RESPONSE_HEADER, &response);
          if (e == REACTOR_ABORT)
            return REACTOR_ABORT;

          value = reactor_http_header_get(response.header, response.headers, "Content-Length");
          if (value)
            {
              http->remaining = strtoul(value->iov_base, NULL, 0);
              http->state = REACTOR_HTTP_STATE_DATA;
              break;
            }

          value = reactor_http_header_get(response.header, response.headers, "Transfer-Encoding");
          if (reactor_http_header_value(value, "chunked"))
            {
              http->state = REACTOR_HTTP_STATE_CHUNK;
              break;
            }

          return reactor_http_error(http);
        case REACTOR_HTTP_STATE_DATA:
          n = MIN(data->iov_len, http->remaining);
          if (n)
            {
              e = reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_RESPONSE_DATA,
                                        (struct iovec []){{.iov_base = data->iov_base, .iov_len = n}});
              if (e != REACTOR_OK)
                return REACTOR_ABORT;
              data->iov_base = (char *) data->iov_base + n;
              data->iov_len -= n;
              http->remaining -= n;
            }

          if (!http->remaining)
            {
              e = reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_RESPONSE_END, NULL);
              if (e != REACTOR_OK)
                return REACTOR_ABORT;
              http->state = REACTOR_HTTP_STATE_HEADER;
            }

          if (!data->iov_len)
            return REACTOR_OK;
          break;
        case REACTOR_HTTP_STATE_CHUNK:
          n = reactor_http_parse_chunk(data, &chunk);
          if (reactor_unlikely(n <= 0))
            return n == 0 ? REACTOR_OK : reactor_http_error(http);

          if (chunk.iov_len)
            e = reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_RESPONSE_DATA, &chunk);
          else
            {
              http->state = REACTOR_HTTP_STATE_HEADER;
              e = reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_RESPONSE_END, NULL);
            }
          if (e != REACTOR_OK)
            return REACTOR_ABORT;

          data->iov_base = (char *) data->iov_base + n;
          data->iov_len -= n;
          if (!data->iov_len)
            return REACTOR_OK;
          break;
        default:
          return reactor_http_error(http);
        }
    }
}

static int reactor_http_client_read(reactor_http *http, struct iovec *data)
{
  reactor_http_response response;
  ssize_t n;
  int e;

  while (data->iov_len)
    {
      n = reactor_http_parse_response(data, &response);
      if (reactor_unlikely(n <= 0))
        return n == 0 ? REACTOR_OK : REACTOR_ERROR;

      e = reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_RESPONSE, &response);
      if (e != REACTOR_OK)
        return REACTOR_ABORT;

      data->iov_base = (char *) data->iov_base + n;
      data->iov_len -= n;
    }

  return REACTOR_OK;
}

static int reactor_http_client_event(void *state, int type, void *data)
{
  reactor_http *http = state;

  if (reactor_likely(type == REACTOR_STREAM_EVENT_READ))
    return reactor_http_client_read(http, data);
  switch (type)
    {
    case REACTOR_STREAM_EVENT_CLOSE:
      return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_CLOSE, NULL);
    case REACTOR_STREAM_EVENT_WRITE:
      return REACTOR_OK;
    default:
      return reactor_http_error(http);
    }
}

static int reactor_http_client_stream_event(void *state, int type, void *data)
{
  reactor_http *http = state;

  if (reactor_likely(type == REACTOR_STREAM_EVENT_READ))
    return reactor_http_client_stream(http, data);
  switch (type)
    {
    case REACTOR_STREAM_EVENT_CLOSE:
      return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_CLOSE, NULL);
    case REACTOR_STREAM_EVENT_WRITE:
      return REACTOR_OK;
    default:
      return reactor_http_error(http);
    }
}

/* reactor_http_date */

void reactor_http_date(char *date)
{
  static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  time_t t;
  struct tm tm;

  (void) time(&t);
  (void) gmtime_r(&t, &tm);
  (void) strftime(date, 30, "---, %d --- %Y %H:%M:%S GMT", &tm);
  memcpy(date, days[tm.tm_wday], 3);
  memcpy(date + 8, months[tm.tm_mon], 3);
}

/* reactor_http_header */

struct iovec *reactor_http_header_get(reactor_http_header *header, size_t headers, char *name)
{
  size_t i;

  for (i = 0; i < headers; i ++)
    if (header[i].name.iov_len == strlen(name) &&
        strncasecmp(header[i].name.iov_base, name, strlen(name)) == 0)
      return &header[i].value;
  return NULL;
}

int reactor_http_header_value(struct iovec *value, char *string)
{
  return value && value->iov_len == strlen(string) && strncasecmp(value->iov_base, string, strlen(string)) == 0;
}

/* reactor_http */

int reactor_http_open(reactor_http *http, reactor_user_callback *callback, void *state, int fd, int flags)
{
  reactor_user_construct(&http->user, callback, state);
  http->state = REACTOR_HTTP_STATE_HEADER;
  http->remaining = 0;
  switch (flags)
    {
    case REACTOR_HTTP_FLAG_CLIENT:
      return reactor_stream_open(&http->stream, reactor_http_client_event, http, fd);
    case REACTOR_HTTP_FLAG_CLIENT | REACTOR_HTTP_FLAG_STREAM:
      return reactor_stream_open(&http->stream, reactor_http_client_stream_event, http, fd);
    case REACTOR_HTTP_FLAG_SERVER:
      return reactor_stream_open(&http->stream, reactor_http_server_event, http, fd);
    case REACTOR_HTTP_FLAG_SERVER | REACTOR_HTTP_FLAG_STREAM:
      return reactor_stream_open(&http->stream, reactor_http_server_stream_event, http, fd);
    default:
      return REACTOR_ERROR;
    }
}

void reactor_http_close(reactor_http *http)
{
  reactor_stream_close(&http->stream);
}

void reactor_http_send_request(reactor_http *http, reactor_http_request *request)
{
  size_t size;
  buffer *buffer;

  size = reactor_http_request_size(request);
  buffer = reactor_stream_buffer(&http->stream);
  buffer_reserve(buffer, buffer_size(buffer) + size);
  reactor_http_request_write(request, (char *) buffer_data(buffer) + buffer_size(buffer));
  buffer->size += size;
}

void reactor_http_send_response(reactor_http *http, reactor_http_response *response)
{
  size_t size;
  buffer *buffer;

  size = reactor_http_response_size(response);
  buffer = reactor_stream_buffer(&http->stream);
  buffer_reserve(buffer, buffer_size(buffer) + size);
  reactor_http_response_write(response, (char *) buffer_data(buffer) + buffer_size(buffer));
  buffer->size += size;
}

int reactor_http_flush(reactor_http *http)
{
  return reactor_stream_flush(&http->stream);
}
