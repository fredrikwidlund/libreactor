#ifndef REACTOR_CORE_H_INCLUDED
#define REACTOR_CORE_H_INCLUDED

#define REACTOR_EVENT          1
#define REACTOR_SCHEDULED_CALL 2

#define REACTOR_MAX_EVENTS     64

#define REACTOR_FLAG_ALLOCATED 0x01
#define REACTOR_FLAG_ACTIVE    0x02
#define REACTOR_FLAG_DELETE    0x04
#define REACTOR_FLAG_EVENT     0x08

typedef struct reactor       reactor;
typedef struct reactor_user  reactor_user;
typedef struct reactor_event reactor_event;
typedef struct reactor_data  reactor_data;
typedef void                 reactor_handler(void *, reactor_event *);

struct reactor
{
  int               flags;
  int               epollfd;
  vector           *schedule;
};

struct reactor_event
{
  void             *source;
  int               type;
  void             *data;
};

struct reactor_user
{
  reactor_handler  *handler;
  void             *state;
};

struct reactor_data
{
  char             *base;
  size_t            size;
};

reactor *reactor_new();
int      reactor_init(reactor *);
int      reactor_delete(reactor *);
int      reactor_run(reactor *);
void     reactor_halt(reactor *);
void     reactor_dispatch(void *, reactor_user *, int, void *);
int      reactor_schedule(reactor *, reactor_user *);

#endif /* REACTOR_CORE_H_INCLUDED */

