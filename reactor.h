#ifndef REACTOR_H_INCLUDED
#define REACTOR_H_INCLUDED

#define REACTOR_EVENTS    64
#define REACTOR_FLAG_HALT 0x01

typedef struct reactor reactor;
typedef struct reactor_call reactor_call;
typedef struct reactor_event reactor_event;
typedef struct reactor_data reactor_data;
typedef void reactor_handler(reactor_event *);

struct reactor
{
  int              epollfd;
  int              flags;
};

struct reactor_event
{
  reactor_call    *call;
  int              type;
  void            *data;
};

struct reactor_call
{
  reactor_handler *handler;
  void            *data;
  /* XXX add call object property? */
};

struct reactor_data
{
  char            *base;
  size_t           size;
};

reactor *reactor_new();
int      reactor_construct(reactor *);
int      reactor_destruct(reactor *);
int      reactor_delete(reactor *);
void     reactor_dispatch(reactor_call *, int, void *);
int      reactor_run();
void     reactor_halt(reactor *);

#endif /* REACTOR_H_INCLUDED */

