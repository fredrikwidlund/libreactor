#ifndef REACTOR_HTTP_H_INCLUDED
#define REACTOR_HTTP_H_INCLUDED

#include "data.h"
#include "pointer.h"
#include "stream.h"

typedef struct http_field    http_field;
typedef struct http_request  http_request;
typedef struct http_response http_response;

struct http_field
{
  data       name;
  data       value;
};

struct http_request
{
  data       method;
  data       target;
  int        minor_version;
  http_field fields[16];
  size_t     fields_count;
  data       body;
};

http_field http_field_construct(data, data);
void       http_field_push(pointer *, http_field);
data       http_field_lookup(http_field *, size_t, data);

ssize_t    http_read_request(stream *, data *, data *, http_field *, size_t *);
void       http_write_request(stream *, data, data, data, data, data);
ssize_t    http_read_response(stream *, int *, data *, http_field *, size_t *);
void       http_write_response(stream *, data, data, data, data);

#endif /* REACTOR_HTTP_H_INCLUDED */
