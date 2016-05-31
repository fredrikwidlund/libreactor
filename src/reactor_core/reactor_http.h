#ifndef REACTOR_HTTP_H_INCLUDED
#define REACTOR_HTTP_H_INCLUDED

#define REACTOR_HTTP_MAX_FIELDS 128

enum reactor_http_state
{
  REACTOR_HTTP_CLOSED,
  REACTOR_HTTP_OPEN,
  REACTOR_HTTP_CLOSING,
  REACTOR_HTTP_CLOSE_WAIT,
  REACTOR_HTTP_INVALID
};

enum reactor_http_events
{
  REACTOR_HTTP_ERROR,
  REACTOR_HTTP_REQUEST,
  REACTOR_HTTP_SHUTDOWN,
  REACTOR_HTTP_CLOSE
};

typedef struct reactor_http reactor_http;
struct reactor_http
{
  int                   state;
  reactor_user          user;
  reactor_tcp           tcp;
  size_t                sessions;
};

typedef struct reactor_http_field reactor_http_field;
struct reactor_http_field
{
  char                 *name;
  char                 *value;
};

typedef struct reactor_http_header reactor_http_header;
struct reactor_http_header
{
  int                   version;
  union
  {
    struct
    {
      char             *method;
      char             *path;
    };
    struct
    {
      int               status;
      char             *message;
    };
  };
  size_t                fields_size;
  reactor_http_field    fields[REACTOR_HTTP_MAX_FIELDS];
};

enum reactor_http_session_state
{
  REACTOR_HTTP_MESSAGE_HEADER,
  REACTOR_HTTP_MESSAGE_BODY,
  REACTOR_HTTP_MESSAGE_COMPLETE,
};

enum reactor_http_message_flags
{
  REACTOR_HTTP_MESSAGE_CHUNKED = 0x01
};

typedef struct reactor_http_message reactor_http_message;
struct reactor_http_message
{
  int                   flags;
  void                 *base;
  size_t                header_size;
  reactor_http_header   header;
  size_t                body_size;
  void                 *body;
};

typedef struct reactor_http_session reactor_http_session;
struct reactor_http_session
{
  int                   state;
  reactor_http_message  message;
  reactor_stream        stream;
  reactor_http         *http;
};

/*
typedef struct reactor_http_request reactor_http_request;
struct reactor_http_request
{
  reactor_http_session *session;
  char                 *base;
  char                 *method;
  char                 *path;
  int                   minor_version;
  char                 *host;
  char                 *service;
  char                 *content;
  size_t                content_size;
  size_t                fields_count;
  reactor_http_field    fields[REACTOR_HTTP_MAX_FIELDS];
};
*/

void reactor_http_init(reactor_http *, reactor_user_callback *, void *);
void reactor_http_server(reactor_http *, char *, char *);
void reactor_http_error(reactor_http *);
void reactor_http_close(reactor_http *);
void reactor_http_tcp_event(void *, int, void *);
void reactor_http_session_init(reactor_http_session *, reactor_http *);
void reactor_http_session_error(reactor_http_session *);
void reactor_http_session_close(reactor_http_session *);
void reactor_http_session_event(void *, int, void *);
void reactor_http_session_read(reactor_http_session *, reactor_stream_data *);
void reactor_http_session_read_header(reactor_http_session *, reactor_stream_data *);
void reactor_http_session_read_body(reactor_http_session *, reactor_stream_data *);

#endif /* REACTOR_HTTP_H_INCLUDED */
