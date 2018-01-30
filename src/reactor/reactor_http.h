#ifndef REACTOR_HTTP_H_INCLUDED
#define REACTOR_HTTP_H_INCLUDED

#define REACTOR_HTTP_MAX_HEADERS    32
#define REACTOR_HTTP_MAX_CHUNK_SIZE (1024*1024*1024)

typedef struct reactor_http reactor_http;
typedef struct reactor_http_header reactor_http_header;
typedef struct reactor_http_request reactor_http_request;
typedef struct reactor_http_response reactor_http_response;

enum reactor_http_event
{
  REACTOR_HTTP_EVENT_ERROR,
  REACTOR_HTTP_EVENT_REQUEST,
  REACTOR_HTTP_EVENT_REQUEST_HEADER,
  REACTOR_HTTP_EVENT_REQUEST_DATA,
  REACTOR_HTTP_EVENT_REQUEST_END,
  REACTOR_HTTP_EVENT_RESPONSE,
  REACTOR_HTTP_EVENT_RESPONSE_HEADER,
  REACTOR_HTTP_EVENT_RESPONSE_DATA,
  REACTOR_HTTP_EVENT_RESPONSE_END,
  REACTOR_HTTP_EVENT_CLOSE
};

enum reactor_http_state
{
  REACTOR_HTTP_STATE_HEADER,
  REACTOR_HTTP_STATE_DATA,
  REACTOR_HTTP_STATE_DATA_CLOSE,
  REACTOR_HTTP_STATE_CHUNK,
  REACTOR_HTTP_STATE_CHUNK_TRAIL
};

enum reactor_http_flag
{
  REACTOR_HTTP_FLAG_CLIENT = 0x00,
  REACTOR_HTTP_FLAG_SERVER = 0x01,
  REACTOR_HTTP_FLAG_STREAM = 0x02
};

struct reactor_http
{
  reactor_user        user;
  reactor_stream      stream;
  int                 state;
  ssize_t             remaining;
};

struct reactor_http_header
{
  struct iovec        name;
  struct iovec        value;
};

struct reactor_http_request
{
  struct iovec        method;
  struct iovec        path;
  int                 version;
  size_t              headers;
  reactor_http_header header[REACTOR_HTTP_MAX_HEADERS];
  struct iovec        body;
};

struct reactor_http_response
{
  int                 version;
  int                 status;
  struct iovec        reason;
  size_t              headers;
  reactor_http_header header[REACTOR_HTTP_MAX_HEADERS];
  struct iovec        body;
};

/* reactor_http_date */

void          reactor_http_date(char *);

/* reactor_http_header */

struct iovec *reactor_http_header_get(reactor_http_header *, size_t, char *);
int           reactor_http_header_value(struct iovec *, char *);

/* reactor_http */

int           reactor_http_open(reactor_http *, reactor_user_callback *, void *, int, int);
void          reactor_http_close(reactor_http *);
void          reactor_http_send_response(reactor_http *, reactor_http_response *);
void          reactor_http_send_request(reactor_http *, reactor_http_request *);
int           reactor_http_flush(reactor_http *);

#endif /* REACTOR_HTTP_H_INCLUDED */
