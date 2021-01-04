#ifndef REACTOR_HTTP_H_INCLUDED
#define REACTOR_HTTP_H_INCLUDED

#define HTTP_MAX_HEADERS 16

typedef struct http_header   http_header;
typedef struct http_headers  http_headers;
typedef struct http_request  http_request;
typedef struct http_response http_response;

struct http_header
{
  segment       name;
  segment       value;
};

struct http_headers
{
  size_t        count;
  http_header   header[HTTP_MAX_HEADERS];
};

struct http_request
{
  segment       method;
  segment       target;
  int           version;
  segment       body;
  http_headers  headers;
};

struct http_response
{
  int          version;
  int          code;
  segment      reason;
  segment      body;
  http_headers headers;
};

void    http_headers_construct(http_headers *);
size_t  http_headers_count(http_headers *);
void    http_headers_add(http_headers *, segment, segment);
segment http_headers_lookup(http_headers *, segment);

ssize_t http_request_read(http_request *, segment);

ssize_t http_response_read(http_response *, segment);
size_t  http_response_size(http_response *);
void    http_response_write(http_response *, segment);
void    http_response_construct(http_response *, int, int, segment, segment, segment);
void    http_response_not_found(http_response *);
void    http_response_ok(http_response *, segment, segment);

#endif /* REACTOR_HTTP_H_INCLUDED */
