#ifndef REACTOR_HTTP_H_INCLUDED
#define REACTOR_HTTP_H_INCLUDED

#ifndef REACTOR_HTTP_HEADERS_MAX
#define REACTOR_HTTP_HEADERS_MAX 32
#endif

enum reactor_http_state
{
  REACTOR_HTTP_STATE_CLOSED     = 0x01,
  REACTOR_HTTP_STATE_CLOSING    = 0x02,
  REACTOR_HTTP_STATE_OPEN       = 0x04
};

enum reactor_http_event
{
  REACTOR_HTTP_EVENT_ERROR,
  REACTOR_HTTP_EVENT_REQUEST,
  REACTOR_HTTP_EVENT_RESPONSE,
  REACTOR_HTTP_EVENT_HANGUP,
  REACTOR_HTTP_EVENT_CLOSE
};

enum reactor_http_flag
{
  REACTOR_HTTP_FLAG_SERVER = 0x01
};

typedef struct reactor_http_header reactor_http_header;
struct reactor_http_header
{
  reactor_memory       name;
  reactor_memory       value;
};

typedef struct reactor_http reactor_http;
struct reactor_http
{
  int                  ref;
  int                  state;
  int                  flags;
  reactor_user         user;
  reactor_stream       stream;
  reactor_http_parser  parser;
};

typedef struct reactor_http_request reactor_http_request;
struct reactor_http_request
{
  reactor_memory       method;
  reactor_memory       path;
  int                  version;
  size_t               header_count;
  reactor_http_header *headers;
  reactor_memory       body;
};

typedef struct reactor_http_response reactor_http_response;
struct reactor_http_response
{
  int                  version;
  int                  status;
  reactor_memory       reason;
  size_t               header_count;
  reactor_http_header *headers;
  reactor_memory       body;
};

void reactor_http_hold(reactor_http *);
void reactor_http_release(reactor_http *);
void reactor_http_open(reactor_http *, reactor_user_callback *, void *, int , int);
void reactor_http_close(reactor_http *);
void reactor_http_write_request(reactor_http *, reactor_http_request *);
void reactor_http_write_response(reactor_http *, reactor_http_response *);
void reactor_http_write_request_line(reactor_http *, reactor_memory, reactor_memory, int);
void reactor_http_write_status_line(reactor_http *, int, int, reactor_memory);
void reactor_http_write_headers(reactor_http *, reactor_http_header *, size_t);
void reactor_http_write_content_length(reactor_http *, size_t);
void reactor_http_write_end(reactor_http *);
void reactor_http_write_body(reactor_http *, reactor_memory);
void reactor_http_flush(reactor_http *);

#endif /* REACTOR_HTTP_H_INCLUDED */
