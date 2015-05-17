#ifndef REACTOR_H_INCLUDED
#define REACTOR_H_INCLUDED

#define REACTOR_EVENTS    64

#define REACTOR_FLAG_HALT          0x01
#define REACTOR_FLAG_EVENT_CONTEXT 0x02

typedef struct reactor       reactor;
typedef struct reactor_call  reactor_call;
typedef struct reactor_event reactor_event;
typedef struct reactor_data  reactor_data;
typedef void                 reactor_handler(reactor_event *);

struct reactor
{
  int               epollfd;
  int               flags;
  vector           *scheduled_calls;
};

struct reactor_event
{
  void             *sender;
  reactor_call     *receiver;
  int               type;
  void             *message;
};

struct reactor_call
{
  void             *object;
  reactor_handler  *handler;
  void             *data;
};

struct reactor_data
{
  char             *base;
  size_t            size;
};

reactor *reactor_new();
int      reactor_construct(reactor *);
int      reactor_destruct(reactor *);
int      reactor_delete(reactor *);
int      reactor_run();
void     reactor_halt(reactor *);
void     reactor_dispatch_call(void *, reactor_call *, int, void *);
int      reactor_schedule_call(reactor *, reactor_call *);

#endif /* REACTOR_H_INCLUDED */

