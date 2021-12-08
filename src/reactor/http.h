#ifndef REACTOR_HTTP_H_INCLUDED
#define REACTOR_HTTP_H_INCLUDED

#include "picohttpparser/picohttpparser.h"
#include "string.h"
#include "stream.h"

typedef struct http_field    http_field;
typedef struct http_request  http_request;
typedef struct http_response http_response;

struct http_field
{
  string      name;
  string      value;
};

struct http_request
{
  string      method;
  string      target;
  int         minor_version;
  http_field *fields[16];
  size_t      fields_count;
  string      body;
};

/*
struct http_response
{
  int               minor_version;
  int               code;
  char             *reason;
  vector            fields;
  void             *body;
  size_t            size;
};
*/

http_field http_field_constant(string, string);
void       http_field_push(http_field, char **);

ssize_t    http_request_parse(http_request *, void *, size_t);

void       http_write_ok_response(stream *, string, string, const void *, size_t);

void       http_response_construct(http_response *);
void       http_response_destruct(http_response *);
void       http_response_set(http_response *, int, char *, void *, size_t);
void       http_response_add(http_response *, char *, char *);
void       http_response_write(http_response *, void *);
size_t     http_response_size(http_response *);

#endif /* REACTOR_HTTP_H_INCLUDED */
