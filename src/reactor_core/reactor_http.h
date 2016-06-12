#ifndef REACTOR_HTTP_H_INCLUDED
#define REACTOR_HTTP_H_INCLUDED

#define REACTOR_HTTP_MAX_HEADERS 32

enum reactor_http_state
{
  REACTOR_HTTP_CLOSED,
  REACTOR_HTTP_OPEN,
  REACTOR_HTTP_CLOSE_WAIT,
  REACTOR_HTTP_INVALID
};

enum reactor_http_events
{
  REACTOR_HTTP_ERROR,
  REACTOR_HTTP_SESSION,
  REACTOR_HTTP_MESSAGE,
  REACTOR_HTTP_SHUTDOWN,
  REACTOR_HTTP_CLOSE
};

enum reactor_http_flags
{
  REACTOR_HTTP_SERVER = 0x01
};

typedef struct reactor_http reactor_http;
struct reactor_http
{
  int                   state;
  int                   flags;
  int                   ref;
  reactor_user          user;
  reactor_tcp           tcp;
};

typedef struct reactor_http_header reactor_http_header;
struct reactor_http_header
{
  char                 *name;
  char                 *value;
};

enum reactor_http_message_type
{
  REACTOR_HTTP_MESSAGE_REQUEST,
  REACTOR_HTTP_MESSAGE_RESPONSE
};

typedef struct reactor_http_message reactor_http_message;
struct reactor_http_message
{
  int                   type;
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
      char             *reason;
    };
  };
  size_t                header_size;
  reactor_http_header  *header;
  size_t                body_size;
  void                 *body;
};

enum reactor_http_session_state
{
  REACTOR_HTTP_SESSION_CLOSED,
  REACTOR_HTTP_SESSION_OPEN,
  REACTOR_HTTP_SESSION_CLOSE_WAIT
};

enum reactor_http_session_flags
{
  REACTOR_HTTP_MESSAGE_CHUNKED = 0x01
};

typedef struct reactor_http_session reactor_http_session;
struct reactor_http_session
{
  int                   state;
  int                   flags;
  int                   ref;
  reactor_http         *http;
  reactor_stream        stream;
  reactor_http_message  message;
  void                 *message_base;
  size_t                body_offset;
  reactor_http_header   header_storage[REACTOR_HTTP_MAX_HEADERS];
};

void reactor_http_init(reactor_http *, reactor_user_callback *, void *);
void reactor_http_open(reactor_http *, char *, char *, int);
void reactor_http_error(reactor_http *);
void reactor_http_close(reactor_http *);
void reactor_http_event(void *, int, void *);

reactor_http_message reactor_http_message_text(char *);

void reactor_http_session_new(reactor_http_session **, reactor_http *);
void reactor_http_session_init(reactor_http_session *, reactor_http *);
void reactor_http_session_error(reactor_http_session *);
void reactor_http_session_close(reactor_http_session *);
void reactor_http_session_event(void *, int, void *);
void reactor_http_session_message(reactor_http_session *, reactor_http_message *);

#endif /* REACTOR_HTTP_H_INCLUDED */
