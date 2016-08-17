#ifndef REACTOR_REST_H_INCLUDED
#define REACTOR_REST_H_INCLUDED

enum reactor_rest_state
{
  REACTOR_REST_CLOSED,
  REACTOR_REST_OPEN,
  REACTOR_REST_INVALID,
  REACTOR_REST_CLOSE_WAIT
};

enum reactor_rest_event
{
  REACTOR_REST_ERROR,
  REACTOR_REST_REQUEST,
  REACTOR_REST_SHUTDOWN,
  REACTOR_REST_CLOSE
};

enum reactor_rest_flags
{
  REACTOR_REST_ENABLE_CORS = 0x01
};

enum reactor_rest_map_type
{
  REACTOR_REST_MAP_MATCH,
  REACTOR_REST_MAP_REGEX
};

typedef struct reactor_rest reactor_rest;
struct reactor_rest
{
  int                   state;
  int                   flags;
  int                   ref;
  reactor_http          http;
  reactor_user          user;
  reactor_timer         timer;
  char                  date[32];
  vector                maps;
};

typedef struct reactor_rest_request reactor_rest_request;
struct reactor_rest_request
{
  reactor_rest         *rest;
  reactor_http_session *session;
  void                 *match;
};

typedef void reactor_rest_handler(void *, reactor_rest_request *);

typedef struct reactor_rest_map reactor_rest_map;
struct reactor_rest_map
{
  int                   type;
  char                 *method;
  char                 *path;
  reactor_rest_handler *handler;
  void                 *state;
  void                 *regex;
};

void reactor_rest_init(reactor_rest *, reactor_user_callback *, void *);
void reactor_rest_open(reactor_rest *, char *, char *, int);
void reactor_rest_error(reactor_rest *);
void reactor_rest_close(reactor_rest *);
void reactor_rest_http_event(void *, int, void *);
void reactor_rest_timer_event(void *, int, void *);
void reactor_rest_timer_update(reactor_rest *);
void reactor_rest_add_match(reactor_rest *, char *, char *, reactor_rest_handler *, void *);
void reactor_rest_add_regex(reactor_rest *, char *, char *, reactor_rest_handler *, void *);
void reactor_rest_respond_empty(reactor_rest_request *, int, char *);
void reactor_rest_respond_body(reactor_rest_request *, int, char *, char *, size_t, void *);
void reactor_rest_respond_not_found(reactor_rest_request *);
void reactor_rest_respond_text(reactor_rest_request *, char *);

#endif /* REACTOR_REST_H_INCLUDED */
