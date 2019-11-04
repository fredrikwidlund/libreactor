#ifndef REACTOR_REACTOR_COUCH_H_INCLUDED
#define REACTOR_REACTOR_COUCH_H_INCLUDED

enum reactor_couch_event_type
{
 REACTOR_COUCH_EVENT_ERROR,
 REACTOR_COUCH_EVENT_WARNING,
 REACTOR_COUCH_EVENT_STATUS,
 REACTOR_COUCH_EVENT_UPDATE
};

enum reactor_couch_status
{
 REACTOR_COUCH_STATUS_OFFLINE,
 REACTOR_COUCH_STATUS_STALE,
 REACTOR_COUCH_STATUS_ONLINE
};

enum reactor_couch_state
{
 REACTOR_COUCH_STATE_DISCONNECTED,
 REACTOR_COUCH_STATE_CONNECTING,
 REACTOR_COUCH_STATE_UPDATE_SEQ,
 REACTOR_COUCH_STATE_SUBSCRIBE,
 REACTOR_COUCH_STATE_RECEIVE,
 REACTOR_COUCH_STATE_VALUE
};

typedef enum reactor_couch_status  reactor_couch_status;
typedef enum reactor_couch_state   reactor_couch_state;
typedef struct reactor_couch       reactor_couch;

struct reactor_couch
{
  reactor_user          user;
  reactor_couch_status  status;
  reactor_couch_state   state;
  reactor_timer         timer;
  reactor_net           net;
  reactor_http          http;
  char                 *location;
  char                 *node;
  char                 *service;
  char                 *database;
  char                 *id;
  char                 *seq;
  char                 *update_seq;
  uint64_t              heartbeat;
};

void           reactor_couch_construct(reactor_couch *, reactor_user_callback *, void *);
void           reactor_couch_destruct(reactor_couch *);
reactor_status reactor_couch_open(reactor_couch *, char *);
int            reactor_couch_online(reactor_couch *);

#endif /* REACTOR_REACTOR_COUCH_H_INCLUDED */
