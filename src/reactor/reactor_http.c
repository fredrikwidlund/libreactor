#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/param.h>
#include <time.h>

#include <dynamic.h>

#include "picohttpparser/picohttpparser.h"
#include "../reactor.h"

/****************/
/* REACTOR_HTTP */
/****************/

static reactor_vector reactor_http_message_date(reactor_http *http)
{
  time_t t;
  struct tm tm;
  static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  static __thread uint64_t now, last = 0;
  static __thread char date[30] = "Thu, 01 Jan 1970 00:00:00 GMT";

  (void) http;
  now = reactor_core_now();
  if (now - last >= 1000000000)
    {
      last = now;
      (void) time(&t);
      (void) gmtime_r(&t, &tm);
      (void) strftime(date, 30, "---, %d --- %Y %H:%M:%S GMT", &tm);
      memcpy(date, days[tm.tm_wday], 3);
      memcpy(date + 8, months[tm.tm_mon], 3);
    }

  return (reactor_vector){date, 29};
}

static reactor_status reactor_http_error(reactor_http *http, char *reason)
{
  reactor_http_reset(http);
  return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_ERROR, (uintptr_t) reason);
}

// returns -1 if partial, or bytes consumed
static ssize_t reactor_http_chunk(char *data, size_t size, char **chunk_data, size_t *chunk_size)
{
  char *end;
  size_t n;

  end = memchr(data, '\n', size);
  if (!end)
    return -1;
  *chunk_data = end + 1;
  *chunk_size = strtoul(data, NULL, 16);

  n = *chunk_size + (*chunk_data - data) + 2;
  if (size < n)
    return -1;

  return n;
}

// returns -1 if partial, or bytes consumed
static ssize_t reactor_http_dechunk(char *data, size_t size, size_t *dechunked_size)
{
  ssize_t n, offset;
  char *chunk;
  size_t chunk_size;

  *dechunked_size = 0;
  offset = 0;
  while (1)
    {
      n = reactor_http_chunk(data + offset, size - offset, &chunk, &chunk_size);
      if (n == -1)
        return -1;

      offset += n;
      if (!chunk_size)
        break;
    }

  offset = 0;
  while (1)
    {
      n = reactor_http_chunk(data + offset, size - offset, &chunk, &chunk_size);
      offset += n;
      if (!chunk_size)
        break;
      memmove(data + *dechunked_size, chunk, chunk_size);
      *dechunked_size += chunk_size;
    }

  return offset;
}

/************************/
/* reactor_http_headers */
/************************/

static char *reactor_http_headers_write(reactor_http_headers *headers, char *base)
{
  size_t i;

  for (i = 0; i < headers->count; i ++)
    {
      memcpy(base, headers->header[i].name.base, headers->header[i].name.size);
      base += headers->header[i].name.size;
      *base ++ = ':';
      *base ++ = ' ';
      memcpy(base, headers->header[i].value.base, headers->header[i].value.size);
      base += headers->header[i].value.size;
      *base ++ = '\r';
      *base ++ = '\n';
    }

  return base;
}

static size_t reactor_http_headers_size(reactor_http_headers *headers)
{
  size_t size, i;

  size = headers->count * 4;
  for (i = 0; i < headers->count; i ++)
    size += headers->header[i].name.size + headers->header[i].value.size;

  return size;
}

reactor_vector reactor_http_headers_lookup(reactor_http_headers *headers, reactor_vector name)
{
  size_t i;

  for (i = 0; i < headers->count; i ++)
    if (reactor_vector_equal_case(headers->header[i].name, name))
      return headers->header[i].value;
  return (reactor_vector) {0};
}

int reactor_http_headers_match(reactor_http_headers *headers, reactor_vector name, reactor_vector value)
{
  return reactor_vector_equal_case(reactor_http_headers_lookup(headers, name), value);
}

void reactor_http_headers_add(reactor_http_headers *headers, reactor_vector name, reactor_vector value)
{
  headers->header[headers->count] = (reactor_http_header) {.name = name, .value = value};
  headers->count ++;
}

/************************/
/* reactor_http_request */
/************************/

static size_t reactor_http_request_size(reactor_http_request *request)
{
  return 14 + request->method.size + request->target.size +
    request->body.size + reactor_http_headers_size(&request->headers);
}

static char *reactor_http_request_write(reactor_http_request *request, char *base)
{
  memcpy(base, request->method.base, request->method.size);
  base += request->method.size;
  *base++ = ' ';
  memcpy(base, request->target.base, request->target.size);
  base += request->target.size;
  memcpy(base, " HTTP/1.0\r\n", 11);
  base[8] = request->version + '0';
  base += 11;
  base = reactor_http_headers_write(&request->headers, base);
  *base++ = '\r';
  *base++ = '\n';
  memcpy(base, request->body.base, request->body.size);
  base += request->body.size;

  return base;
}

static ssize_t reactor_http_request_parse_head(reactor_http_request *request, char *data, size_t size)
{
  ssize_t n;

  request->body = reactor_vector_data(NULL, 0);
  request->headers.count = REACTOR_HTTP_MAX_HEADERS;
  n = phr_parse_request(data, size,
                        (const char **) &request->method.base, &request->method.size,
                        (const char **) &request->target.base, &request->target.size,
                        &request->version,
                        (struct phr_header *) request->headers.header, &request->headers.count, 0);
  return n == -2 ? 0 : n;
}

// returns -1 if invalid and -2 if chunked
static ssize_t reactor_http_request_parse_body_size(reactor_http_request *request)
{
  reactor_vector value;

  value = reactor_http_headers_lookup(&request->headers, reactor_vector_string("Content-Length"));
  if (value.size)
    return strtoull(value.base, NULL, 10);
  if (reactor_http_headers_match(&request->headers, reactor_vector_string("Transfer-Encoding"),
                                 reactor_vector_string("chunked")))
    return -2;
  return 0;
}

static reactor_status reactor_http_request_parse_complete(reactor_http *http, char *data, size_t size, size_t *consumed)
{
  reactor_http_request request;
  ssize_t n;
  size_t offset = 0;

  *consumed = 0;
  n = reactor_http_request_parse_head(&request, data, size);
  if (n <= 0)
    return n == 0 ? REACTOR_OK : reactor_http_error(http, "invalid request");

  offset += n;
  request.body.base = data + offset;
  n = reactor_http_request_parse_body_size(&request);
  if (n == -1)
    return reactor_http_error(http, "unknown request body size");
  if (n == -2)
    {
      n = reactor_http_dechunk(data + offset, size - offset, &request.body.size);
      if (n == -1)
        return REACTOR_OK;
      offset += n;
    }
  else
    {
      offset += n;
      if (offset > size)
        return REACTOR_OK;;
      request.body.size = n;
    }

  *consumed += offset;
  return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_REQUEST, (uintptr_t) &request);
}

static reactor_status reactor_http_request_handler(reactor_event *event)
{
  reactor_http *http = event->state;
  reactor_status e;
  char *data;
  size_t size, consumed, consumed_more;

  if (event->type == REACTOR_STREAM_EVENT_DATA)
    {
      data = reactor_stream_data(&http->stream);
      size = reactor_stream_size(&http->stream);
      consumed = 0;
      do
        {
          e = reactor_http_request_parse_complete(http, data + consumed, size - consumed, &consumed_more);
          if (e != REACTOR_OK)
            return e;
          consumed += consumed_more;
        }
      while (consumed_more && consumed < size);

      reactor_stream_consume(&http->stream, consumed);
      reactor_stream_flush(&http->stream);
      return REACTOR_OK;
    }

  if (event->type == REACTOR_STREAM_EVENT_CLOSE)
    return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_CLOSE, 0);

  return reactor_http_error(http, "stream error");
}

static reactor_status reactor_http_request_parse_stream(reactor_http *http, char *data, size_t size, size_t *consumed)
{
  reactor_http_request request;
  reactor_vector vector;
  ssize_t n;

  *consumed = 0;
  switch (http->state)
    {
    case REACTOR_HTTP_STATE_HEAD:
      n = reactor_http_request_parse_head(&request, data, size);
      if (n <= 0)
        return n == 0 ? REACTOR_OK : reactor_http_error(http, "invalid request");
      *consumed = n;

      request.body.base = data + *consumed;
      n = reactor_http_request_parse_body_size(&request);
      if (n == -1)
        return reactor_http_error(http, "unknown request body size");
      if (n == -2)
        http->state = REACTOR_HTTP_STATE_CHUNK;
      else
        {
          http->state = REACTOR_HTTP_STATE_BODY;
          http->size = n;
        }
      return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_REQUEST_HEAD, (uintptr_t) &request);
    case REACTOR_HTTP_STATE_BODY:
      *consumed = MIN(http->size, size);
      vector = reactor_vector_data(data, *consumed);
      if (*consumed == 0)
        http->state = REACTOR_HTTP_STATE_HEAD;
      return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_REQUEST_BODY, (uintptr_t) &vector);
    case REACTOR_HTTP_STATE_CHUNK:
      n = reactor_http_chunk(data, size, (char **) &vector.base, &vector.size);
      if (n == -1)
        return REACTOR_OK;
      if (n == 0)
        http->state = REACTOR_HTTP_STATE_HEAD;
      *consumed = n;
      return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_REQUEST_BODY, (uintptr_t) &vector);
    default:
      return REACTOR_ABORT;
    }
}

static reactor_status reactor_http_request_stream_handler(reactor_event *event)
{
  reactor_http *http = event->state;
  reactor_status e;
  char *data;
  size_t size, consumed, consumed_more;

  if (event->type == REACTOR_STREAM_EVENT_DATA)
    {
      data = reactor_stream_data(&http->stream);
      size = reactor_stream_size(&http->stream);

      consumed = 0;
      do
        {
          e = reactor_http_request_parse_stream(http, data + consumed, size - consumed, &consumed_more);
          if (e != REACTOR_OK)
            return e;
          consumed += consumed_more;
        }
      while (consumed_more);

      reactor_stream_consume(&http->stream, consumed);
      reactor_stream_flush(&http->stream);
      return REACTOR_OK;
    }

  if (event->type == REACTOR_STREAM_EVENT_CLOSE)
    return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_CLOSE, 0);

  return reactor_http_error(http, "stream error");
}

void reactor_http_request_construct(reactor_http_request *request, reactor_vector method, reactor_vector target,
                                    int version, reactor_vector body)
{
  request->method = method;
  request->target = target;
  request->version = version;
  request->headers.count = 0;
  request->body = body;
}

/*************************/
/* reactor_http_response */
/*************************/

static size_t reactor_http_response_size(reactor_http_response *response)
{
  return 17 + response->reason.size + response->body.size + reactor_http_headers_size(&response->headers);
}

static char *reactor_http_response_write(reactor_http_response *response, char *base)
{
  memcpy(base, "HTTP/1.0 500 ", 13);
  base[7] = response->version + '0';
  base += 12;
  reactor_utility_u32sprint(response->code, base);
  base ++;
  memcpy(base, response->reason.base, response->reason.size);
  base += response->reason.size;
  *base ++ = '\r';
  *base ++ = '\n';
  base = reactor_http_headers_write(&response->headers, base);
  *base++ = '\r';
  *base++ = '\n';
  memcpy(base, response->body.base, response->body.size);
  base += response->body.size;

  return base;
}

static ssize_t reactor_http_response_parse_head(reactor_http_response *response, char *data, size_t size)
{
  ssize_t n;

  response->body = reactor_vector_data(NULL, 0);
  response->headers.count = REACTOR_HTTP_MAX_HEADERS;
  n = phr_parse_response(data, size,
                         &response->version, &response->code,
                         (const char **) &response->reason.base, &response->reason.size,
                         (struct phr_header *) response->headers.header, &response->headers.count, 0);
  return n == -2 ? 0 : n;
}

// returns -1 if invalid and -2 if chunked
static ssize_t reactor_http_response_parse_body_size(reactor_http_response *response)
{
  reactor_vector value;

  value = reactor_http_headers_lookup(&response->headers, reactor_vector_string("Content-Length"));
  if (value.size)
    return strtoull(value.base, NULL, 10);
  if (reactor_http_headers_match(&response->headers, reactor_vector_string("Transfer-Encoding"),
                                 reactor_vector_string("chunked")))
    return -2;
  return -1;
}

static reactor_status reactor_http_response_parse_complete(reactor_http *http, char *data, size_t size,
                                                           size_t *consumed)
{
  reactor_http_response response;
  ssize_t n;
  size_t offset = 0;

  *consumed = 0;
  n = reactor_http_response_parse_head(&response, data, size);
  if (n <= 0)
    return n == 0 ? REACTOR_OK : reactor_http_error(http, "invalid response");

  offset += n;
  response.body.base = data + offset;
  n = reactor_http_response_parse_body_size(&response);
  if (n == -1)
    return reactor_http_error(http, "unknown response body size");
  if (n == -2)
    {
      n = reactor_http_dechunk(data + offset, size - offset, &response.body.size);
      if (n == -1)
        return REACTOR_OK;
      offset += n;
    }
  else
    {
      offset += n;
      if (offset > size)
        return REACTOR_OK;;
      response.body.size = n;
    }

  *consumed += offset;
  return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_RESPONSE, (uintptr_t) &response);
}

static reactor_status reactor_http_response_handler(reactor_event *event)
{
  reactor_http *http = event->state;
  reactor_status e;
  char *data;
  size_t size, consumed, consumed_more;

  if (event->type == REACTOR_STREAM_EVENT_DATA)
    {
      data = reactor_stream_data(&http->stream);
      size = reactor_stream_size(&http->stream);
      consumed = 0;
      do
        {
          e = reactor_http_response_parse_complete(http, data + consumed, size - consumed, &consumed_more);
          if (e != REACTOR_OK)
            return e;
          consumed += consumed_more;
        }
      while (consumed_more && consumed < size);

      reactor_stream_consume(&http->stream, consumed);
      return REACTOR_OK;
    }

  if (event->type == REACTOR_STREAM_EVENT_CLOSE)
    return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_CLOSE, 0);

  return reactor_http_error(http, "stream error");
}

static reactor_status reactor_http_response_parse_stream(reactor_http *http, char *data, size_t size, size_t *consumed)
{
  reactor_http_response response;
  reactor_vector vector;
  ssize_t n;

  *consumed = 0;
  switch (http->state)
    {
    case REACTOR_HTTP_STATE_HEAD:
      n = reactor_http_response_parse_head(&response, data, size);
      if (n <= 0)
        return n == 0 ? REACTOR_OK : reactor_http_error(http, "invalid response");
      *consumed = n;

      response.body.base = data + *consumed;
      n = reactor_http_response_parse_body_size(&response);
      if (n == -1)
        return reactor_http_error(http, "unknown response body size");
      if (n == -2)
        http->state = REACTOR_HTTP_STATE_CHUNK;
      else
        {
          http->state = REACTOR_HTTP_STATE_BODY;
          http->size = n;
        }
      return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_RESPONSE_HEAD, (uintptr_t) &response);
    case REACTOR_HTTP_STATE_BODY:
      *consumed = MIN(http->size, size);
      vector = reactor_vector_data(data, *consumed);
      if (*consumed == 0)
        http->state = REACTOR_HTTP_STATE_HEAD;
      return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_RESPONSE_BODY, (uintptr_t) &vector);
    case REACTOR_HTTP_STATE_CHUNK:
      n = reactor_http_chunk(data, size, (char **) &vector.base, &vector.size);
      if (n == -1)
        return REACTOR_OK;
      if (n == 0)
        http->state = REACTOR_HTTP_STATE_HEAD;
      *consumed = n;
      return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_RESPONSE_BODY, (uintptr_t) &vector);
    default:
      return REACTOR_ABORT;
    }
}

static reactor_status reactor_http_response_stream_handler(reactor_event *event)
{
  reactor_http *http = event->state;
  reactor_status e;
  char *data;
  size_t size, consumed, consumed_more;

  if (event->type == REACTOR_STREAM_EVENT_DATA)
    {
      data = reactor_stream_data(&http->stream);
      size = reactor_stream_size(&http->stream);

      consumed = 0;
      do
        {
          e = reactor_http_response_parse_stream(http, data + consumed, size - consumed, &consumed_more);
          if (e != REACTOR_OK)
            return e;
          consumed += consumed_more;
        }
      while (consumed_more);

      reactor_stream_consume(&http->stream, consumed);
      return REACTOR_OK;
    }

  if (event->type == REACTOR_STREAM_EVENT_CLOSE)
    return reactor_user_dispatch(&http->user, REACTOR_HTTP_EVENT_CLOSE, 0);

  return reactor_http_error(http, "stream error");
}

/****************/
/* reactor_http */
/****************/

void reactor_http_construct(reactor_http *http, reactor_user_callback *callback, void *state)
{
  *http = (reactor_http) {0};
  reactor_user_construct(&http->user, callback, state);
  reactor_stream_construct(&http->stream, reactor_http_response_handler, http);
  reactor_http_set_mode(http, REACTOR_HTTP_MODE_RESPONSE);
}

void reactor_http_destruct(reactor_http *http)
{
  reactor_stream_destruct(&http->stream);
  free(http->authority.base);
  http->authority.base = NULL;
}

void reactor_http_open(reactor_http *http, int fd)
{
  reactor_stream_open(&http->stream, fd);
}

reactor_status reactor_http_flush(reactor_http *http)
{
  return reactor_stream_flush(&http->stream);
}

void reactor_http_reset(reactor_http *http)
{
  reactor_stream_reset(&http->stream);
  reactor_http_set_mode(http, REACTOR_HTTP_MODE_RESPONSE);
}

void reactor_http_shutdown(reactor_http *http)
{
  reactor_stream_shutdown(&http->stream);
}

void reactor_http_set_authority(reactor_http *http, reactor_vector node, reactor_vector service)
{
  free(http->authority.base);
  if (reactor_vector_equal(service, reactor_vector_string("80")) ||
      reactor_vector_equal_case(service, reactor_vector_string("http")))
   {
      http->authority.size = node.size;
      http->authority.base = malloc(http->authority.size);
      if (!http->authority.base)
        abort();
      memcpy(http->authority.base, node.base, node.size);
    }
  else
    {
      http->authority.size = node.size + 1 + service.size;
      http->authority.base = malloc(http->authority.size);
      if (!http->authority.base)
        abort();
      memcpy(http->authority.base, node.base, node.size);
      ((char *) http->authority.base)[node.size] = ':';
      memcpy((char *) http->authority.base + node.size + 1, service.base, service.size);
    }
}

void reactor_http_set_mode(reactor_http *http, reactor_http_mode mode)
{
  http->state = REACTOR_HTTP_STATE_HEAD;
  http->size = 0;
  switch (mode)
    {
    case REACTOR_HTTP_MODE_RESPONSE:
      reactor_stream_user(&http->stream, reactor_http_response_handler, http);
      break;
    case REACTOR_HTTP_MODE_RESPONSE_STREAM:
      reactor_stream_user(&http->stream, reactor_http_response_stream_handler, http);
      break;
    case REACTOR_HTTP_MODE_REQUEST:
      reactor_stream_user(&http->stream, reactor_http_request_handler, http);
      break;
    case REACTOR_HTTP_MODE_REQUEST_STREAM:
      reactor_stream_user(&http->stream, reactor_http_request_stream_handler, http);
      break;
    }
}

void reactor_http_write_request(reactor_http *http, reactor_http_request *request)
{
  (void) reactor_http_request_write(request, reactor_stream_segment(&http->stream, reactor_http_request_size(request)));
}

void reactor_http_create_request(reactor_http *http, reactor_http_request *request,
                                 reactor_vector method, reactor_vector target, int version,
                                 reactor_vector type, size_t length, reactor_vector body)
{
  *request = (reactor_http_request)
    {
     .method = method,
     .target = target,
     .version = version,
     .headers.count = 1,
     .headers.header[0] = {reactor_vector_string("Host"), http->authority},
     .body = body
    };

  if (type.size)
    {
      reactor_http_headers_add(&request->headers, reactor_vector_string("Content-Type"), type);
      reactor_http_headers_add(&request->headers, reactor_vector_string("Content-Length"), reactor_utility_u32tov(length));
    }
}

void reactor_http_get(reactor_http *http, reactor_vector target)
{
  reactor_http_request request;

  reactor_http_create_request(http, &request, reactor_vector_string("GET"), target, 1,
                              reactor_vector_empty(), 0, reactor_vector_empty());
  reactor_http_write_request(http, &request);
  reactor_http_flush(http);
}

void reactor_http_write_response(reactor_http *http, reactor_http_response *response)
{
  (void) reactor_http_response_write(response, reactor_stream_segment(&http->stream,
                                                                      reactor_http_response_size(response)));
}

void reactor_http_create_response(reactor_http *http, reactor_http_response *response,
                                  int version, int code, reactor_vector reason,
                                  reactor_vector type, size_t length, reactor_vector body)
{
  *response = (reactor_http_response)
    {
     .version = version,
     .code = code,
     .reason = reason,
     .headers.count = 2,
     .headers.header[0] = {reactor_vector_string("Server"), reactor_vector_string("libreactor")},
     .headers.header[1] = {reactor_vector_string("Date"), reactor_http_message_date(http)},
     .body = body
    };

  if (type.size)
    reactor_http_headers_add(&response->headers, reactor_vector_string("Content-Type"), type);
  if (code < 100 || (code >= 200 && code != 204 && code != 304))
    reactor_http_headers_add(&response->headers, reactor_vector_string("Content-Length"),
                             reactor_utility_u32tov(length));
}
