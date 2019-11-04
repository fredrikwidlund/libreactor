#ifndef REACTOR_REACTOR_HTTP_H_INCLUDED
#define REACTOR_REACTOR_HTTP_H_INCLUDED

#define REACTOR_HTTP_BLOCK_SIZE  16384
#define REACTOR_HTTP_MAX_HEADERS 32

enum reactor_http_event
{
 REACTOR_HTTP_EVENT_ERROR,
 REACTOR_HTTP_EVENT_CLOSE,
 REACTOR_HTTP_EVENT_RESPONSE,
 REACTOR_HTTP_EVENT_RESPONSE_HEAD,
 REACTOR_HTTP_EVENT_RESPONSE_BODY,
 REACTOR_HTTP_EVENT_REQUEST,
 REACTOR_HTTP_EVENT_REQUEST_HEAD,
 REACTOR_HTTP_EVENT_REQUEST_BODY
};

enum reactor_http_mode
{
 REACTOR_HTTP_MODE_RESPONSE,
 REACTOR_HTTP_MODE_RESPONSE_STREAM,
 REACTOR_HTTP_MODE_REQUEST,
 REACTOR_HTTP_MODE_REQUEST_STREAM
};

enum reactor_http_state
{
 REACTOR_HTTP_STATE_HEAD,
 REACTOR_HTTP_STATE_BODY,
 REACTOR_HTTP_STATE_CHUNK
};

typedef struct reactor_http_header   reactor_http_header;
typedef struct reactor_http_headers  reactor_http_headers;
typedef struct reactor_http_request  reactor_http_request;
typedef struct reactor_http_response reactor_http_response;
typedef struct reactor_http          reactor_http;
typedef enum reactor_http_mode       reactor_http_mode;
typedef enum reactor_http_state      reactor_http_state;

struct reactor_http_header
{
  reactor_vector       name;
  reactor_vector       value;
};

struct reactor_http_headers
{
  size_t               count;
  reactor_http_header  header[REACTOR_HTTP_MAX_HEADERS];
};

struct reactor_http_response
{
  int                  version;
  int                  code;
  reactor_vector       reason;
  reactor_vector       body;
  reactor_http_headers headers;
};

struct reactor_http_request
{
  reactor_vector       method;
  reactor_vector       target;
  int                  version;
  reactor_vector       body;
  reactor_http_headers headers;
};

struct reactor_http
{
  reactor_user         user;
  reactor_stream       stream;
  reactor_vector       authority;
  reactor_http_state   state;
  size_t               size;
};

reactor_vector reactor_http_headers_lookup(reactor_http_headers *, reactor_vector);
int            reactor_http_headers_match(reactor_http_headers *, reactor_vector, reactor_vector);
void           reactor_http_headers_add(reactor_http_headers *, reactor_vector, reactor_vector);

void           reactor_http_request_construct(reactor_http_request *, reactor_vector, reactor_vector, int, reactor_vector);

void           reactor_http_construct(reactor_http *, reactor_user_callback *, void *);
void           reactor_http_destruct(reactor_http *);
void           reactor_http_open(reactor_http *, int);
reactor_status reactor_http_flush(reactor_http *);
void           reactor_http_reset(reactor_http *);
void           reactor_http_shutdown(reactor_http *);
void           reactor_http_set_authority(reactor_http *, reactor_vector, reactor_vector);
void           reactor_http_set_mode(reactor_http *, reactor_http_mode);
void           reactor_http_create_request(reactor_http *, reactor_http_request *, reactor_vector, reactor_vector, int,
                                           reactor_vector, size_t, reactor_vector);
void           reactor_http_write_request(reactor_http *, reactor_http_request *);
void           reactor_http_create_response(reactor_http *, reactor_http_response *, int, int, reactor_vector,
                                            reactor_vector, size_t, reactor_vector);
void           reactor_http_write_response(reactor_http *, reactor_http_response *);

void           reactor_http_get(reactor_http *, reactor_vector);

#endif /* REACTOR_REACTOR_HTTP_H_INCLUDED */
