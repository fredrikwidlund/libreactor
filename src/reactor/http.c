#include <stdio.h>
#include <string.h>

#include "picohttpparser/picohttpparser.h"
#include "http.h"
#include "utility.h"
#include "stream.h"

/* http_field */

http_field http_field_constant(string name, string value)
{
  return (http_field) {.name = name, .value = value};
}

void http_field_push(http_field field, char **base)
{
  string_push(field.name, base);
  string_push_char(':', base);
  string_push_char(' ', base);
  string_push(field.value, base);
  string_push_char('\r', base);
  string_push_char('\n', base);
}

/* http_request */

ssize_t http_request_parse(http_request *request, void *base, size_t size)
{
  int n;

  request->fields_count = 16;
  n = phr_parse_request(base, size,
                        &request->method.base, &request->method.size,
                        &request->target.base, &request->target.size,
                        &request->minor_version, (struct phr_header *) request->fields, &request->fields_count, 0);
  return n;
}

/* http_response */

void http_write_ok_response(stream *stream, string date, string type, const void *body, size_t size)
{
  char string_storage[16];
  string length = string_integer(size, string_storage);
  char *base = stream_allocate(stream, 17 + 17 + 37 + (16 + string_size(type)) + (18 + string_size(length)) + 2 + size);

  string_push(string_constant("HTTP/1.1 200 OK\r\n"), &base);
  http_field_push(http_field_constant(string_constant("Server"), string_constant("reactor")), &base);
  http_field_push(http_field_constant(string_constant("Date"), date), &base);
  http_field_push(http_field_constant(string_constant("Content-Type"), type), &base);
  http_field_push(http_field_constant(string_constant("Content-Length"), length), &base);
  string_push(string_constant("\r\n"), &base);
  memcpy(base, body, size);
}
