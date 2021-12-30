#include <stdio.h>
#include <string.h>

#include "picohttpparser/picohttpparser.h"
#include "data.h"
#include "pointer.h"
#include "http.h"
#include "utility.h"
#include "stream.h"

/* http_field */

http_field http_field_construct(data name, data value)
{
  return (http_field) {.name = name, .value = value};
}

void http_field_push(pointer *pointer, http_field field)
{
  pointer_push(pointer, field.name);
  pointer_push_byte(pointer, ':');
  pointer_push_byte(pointer, ' ');
  pointer_push(pointer, field.value);
  pointer_push_byte(pointer, '\r');
  pointer_push_byte(pointer, '\n');
}

/* http_request */

ssize_t http_request_parse(http_request *request, data data)
{
  int n;

  request->fields_count = 16;
  n = phr_parse_request(data_base(data), data_size(data),
                        (const char **) &request->method.base, &request->method.size,
                        (const char **) &request->target.base, &request->target.size,
                        &request->minor_version, (struct phr_header *) request->fields, &request->fields_count, 0);
  return n;
}

/* http_response */

void http_write_response(stream *stream, data status, data date, data type, data body)
{
  char storage[16];
  size_t size;
  data length;
  pointer p;

  size = utility_u32_len(data_size(body));
  utility_u32_sprint(data_size(body), storage + size);
  length = data_construct(storage, size);

  p = stream_allocate(stream, 9 + data_size(status) + 2 + 17 + 37 +
                      (data_empty(type) ? 0 : 16 + data_size(type)) +
                      (18 + data_size(length)) + 2 + data_size(body));
  pointer_push(&p, data_string("HTTP/1.1 "));
  pointer_push(&p, status);
  pointer_push(&p, data_string("\r\n"));
  http_field_push(&p, http_field_construct(data_string("Server"), data_string("reactor")));
  http_field_push(&p, http_field_construct(data_string("Date"), date));
  if (!data_empty(type))
    http_field_push(&p, http_field_construct(data_string("Content-Type"), type));
  http_field_push(&p, http_field_construct(data_string("Content-Length"), length));
  pointer_push(&p, data_string("\r\n"));
  pointer_push(&p, body);
}

void http_write_ok_response(stream *stream, data date, data type, data body)
{
  char storage[16];
  size_t size;
  data length;
  pointer p;

  size = utility_u32_len(data_size(body));
  utility_u32_sprint(data_size(body), storage + size);
  length = data_construct(storage, size);

  p = stream_allocate(stream, 17 + 17 + 37 + (16 + data_size(type)) + (18 + data_size(length)) + 2 + data_size(body));
  pointer_push(&p, data_string("HTTP/1.1 200 OK\r\n"));
  http_field_push(&p, http_field_construct(data_string("Server"), data_string("reactor")));
  http_field_push(&p, http_field_construct(data_string("Date"), date));
  http_field_push(&p, http_field_construct(data_string("Content-Type"), type));
  http_field_push(&p, http_field_construct(data_string("Content-Length"), length));
  pointer_push(&p, data_string("\r\n"));
  pointer_push(&p, body);
}
