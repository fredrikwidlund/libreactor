#include <stdio.h>
#include <string.h>

#include "picohttpparser/picohttpparser.h"
#include "reactor.h"

/* http field */

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

data http_field_lookup(http_field *fields, size_t fields_count, data name)
{
  size_t i;

  for (i = 0; i < fields_count; i++)
  {
    if (data_size(fields[i].name) == data_size(name) &&
        strncasecmp(data_base(fields[i].name), data_base(name), data_size(name)) == 0)
      return fields[i].value;
  }
  return data_null();
}

/* http request */

ssize_t http_read_request(stream *stream, data *method, data *target, http_field *fields, size_t *fields_count)
{
  int n, minor_version;

  n = phr_parse_request(stream->input.data, stream->input.size,
                        (const char **) &method->base, &method->size,
                        (const char **) &target->base, &target->size,
                        &minor_version,
                        (struct phr_header *) fields, fields_count, 0);
  asm volatile("": : :"memory");
  return n;
}

void http_write_request(stream *stream, data method, data target, data host, data type, data body)
{
  char storage[16];
  size_t size;
  data length;
  pointer p;

  size = utility_u32_len(data_size(body));
  utility_u32_sprint(data_size(body), storage + size);
  length = data_construct(storage, size);

  p = stream_allocate(stream,
                      data_size(method) + 1 + data_size(target) + 11 +
                      6 + data_size(host) + 2 +
                      (data_size(body) ?
                       14 + data_size(type) + 2 +
                       16 + data_size(length) + 2 : 0) +
                      2 +
                      data_size(body));
  pointer_push(&p, method);
  pointer_push_byte(&p, ' ');
  pointer_push(&p, target);
  pointer_push(&p, data_string(" HTTP/1.1\r\n"));
  http_field_push(&p, http_field_construct(data_string("Host"), host));
  if (data_size(body))
  {
    http_field_push(&p, http_field_construct(data_string("Content-Type"), type));
    http_field_push(&p, http_field_construct(data_string("Content-Length"), length));
  }
  pointer_push(&p, data_string("\r\n"));
  pointer_push(&p, body);
}

/* http response */

ssize_t http_read_response(stream *stream, int *status_code, data *status, http_field *fields, size_t *fields_count)
{
  int n, minor_version;

  n = phr_parse_response(stream->input.data, stream->input.size, &minor_version, status_code,
                         (const char **) &status->base, &status->size,
                         (struct phr_header *) fields, fields_count, 0);
  asm volatile("": : :"memory");
  return n;  
}

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
