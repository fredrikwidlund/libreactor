#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <dynamic.h>
#include "picohttpparser/picohttpparser.h"

#include "reactor.h"

segment http_date(int update)
{
  time_t t;
  struct tm tm;
  static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  static __thread char date[30] = "Thu, 01 Jan 1970 00:00:00 GMT";

  if (update)
    {
      (void) time(&t);
      (void) gmtime_r(&t, &tm);
      (void) strftime(date, 30, "---, %d --- %Y %H:%M:%S GMT", &tm);
      memcpy(date, days[tm.tm_wday], 3);
      memcpy(date + 8, months[tm.tm_mon], 3);
    }

  return (segment) {date, 29};
}

static void http_headers_add(http_headers *headers, segment name, segment value)
{
  if (headers->count < HTTP_MAX_HEADERS)
    {
      headers->header[headers->count] = (http_header) {name, value};
      headers->count ++;
    }
}

static size_t http_headers_size(http_headers *headers)
{
  size_t size, i;

  size = headers->count * 4;
  for (i = 0; i < headers->count; i ++)
    size += headers->header[i].name.size + headers->header[i].value.size;

  return size;
}

static char *http_headers_write(http_headers *headers, char *base)
{
  http_header *h = headers->header;
  size_t i = 0, l1, l2;

  for (i = 0; i < headers->count; i ++)
    {
      l1 = h[i].name.size;
      l2 = h[i].value.size;
      memcpy(base, h[i].name.base, l1);
      base += l1;
      memcpy(base, ": ", 2);
      base += 2;
      memcpy(base, h[i].value.base, l2);
      base += l2;
      memcpy(base, "\r\n", 2);
      base += 2;
    }
  return base;
}

ssize_t http_request_read(http_request *request, segment segment)
{
  ssize_t n;

  request->body = (struct segment) {0};
  request->headers.count = HTTP_MAX_HEADERS;
  n = phr_parse_request(segment.base, segment.size,
                        (const char **) &request->method.base, &request->method.size,
                        (const char **) &request->target.base, &request->target.size,
                        &request->version,
                        (struct phr_header *) request->headers.header, &request->headers.count, 0);
  return n == -2 ? 0 : n;
}

size_t http_response_size(http_response *response)
{
  return 17 + response->reason.size + response->body.size + http_headers_size(&response->headers);
}

void http_response_write(http_response *response, segment segment)
{
  char *base = segment.base;

  memcpy(base, "HTTP/1.0 000 ", 13);
  base[7] = response->version + '0';
  base += 12;
  utility_u32_sprint(response->code, base);
  base ++;
  memcpy(base, response->reason.base, response->reason.size);
  base += response->reason.size;
  *base ++ = '\r';
  *base ++ = '\n';
  base = http_headers_write(&response->headers, base);
  *base++ = '\r';
  *base++ = '\n';
  memcpy(base, response->body.base, response->body.size);
  base += response->body.size;
}

void http_response_construct(http_response *response,
                             int version, int code, segment reason, segment type, segment body)
{
  *response = (http_response)
    {
     .version = version,
     .code = code,
     .reason = reason,
     .body = body,
     .headers.count = 2,
     .headers.header =
     {
      {segment_string("Server"), segment_string("libreactor")},
      {segment_string("Date"), http_date(0)}
     }
    };

  if (type.size)
    http_headers_add(&response->headers, segment_string("Content-Type"), type);
  if (code < 100 || (code >= 200 && code != 204 && code != 304))
    http_headers_add(&response->headers, segment_string("Content-Length"), utility_u32_segment(body.size));
}

void http_response_ok(http_response *response, segment type, segment body)
{
  http_response_construct(response, 1, 200, segment_string("OK"), type, body);
}

void http_response_not_found(http_response *response)
{
  http_response_construct(response, 1, 404, segment_string("Not Found"), segment_empty(), segment_empty());
}
