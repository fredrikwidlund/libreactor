#ifndef REACTOR_REACTOR_H_INCLUDED
#define REACTOR_REACTOR_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>
#include <sys/epoll.h>

#ifdef UNIT_TESTING
extern void mock_assert(const int, const char * const, const char * const, const int);
#undef assert
#define assert(expression) mock_assert((int)(expression), #expression, __FILE__, __LINE__);
#endif /* UNIT_TESTING */

#define REACTOR_MAX_EVENTS 32

#define reactor_likely(x)   __builtin_expect(!!(x), 1)
#define reactor_unlikely(x) __builtin_expect(!!(x), 0)

enum reactor_event_type
{
  REACTOR_EPOLL_EVENT
};

typedef struct reactor_handler    reactor_handler;
typedef struct reactor_event      reactor_event;
typedef void                      reactor_callback(reactor_event *);
typedef struct reactor            reactor;

struct reactor_handler
{
  reactor_callback   *callback;
  void               *state;
};

struct reactor_event
{
  int                 type;
  void               *state;
  uintptr_t           data;
};

struct reactor
{
  int                 epoll_fd;
  int                 active;
  size_t              descriptors;
  uint64_t            time;
  struct epoll_event *event;
  struct epoll_event *event_end;
};

/* reactor_handler */

void     reactor_handler_construct(reactor_handler *, reactor_callback *, void *);
void     reactor_handler_destruct(reactor_handler *);

/* reactor */

uint64_t reactor_now(void);
void     reactor_construct(void);
void     reactor_destruct(void);
void     reactor_add(reactor_handler *, int, uint32_t);
void     reactor_modify(reactor_handler *, int, uint32_t);
void     reactor_delete(reactor_handler *, int);
void     reactor_dispatch(reactor_handler *, int, uintptr_t);
void     reactor_loop_once(void);
void     reactor_loop(void);
void     reactor_abort(void);

#endif /* REACTOR_REACTOR_H_INCLUDED */
