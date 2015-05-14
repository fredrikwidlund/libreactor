#ifndef REACTOR_TIMER_H_INCLUDED
#define REACTOR_TIMER_H_INCLUDED

#define REACTOR_TIMER_TIMEOUT 1

typedef struct reactor_timer reactor_timer;

struct reactor_timer
{
  reactor_call        user;
  reactor            *reactor;
  int                 descriptor;
  struct epoll_event  ev;
  reactor_call        main;
};

reactor_timer *reactor_timer_new(reactor *, reactor_handler *, uint64_t, void *);
int            reactor_timer_construct(reactor *, reactor_timer *, reactor_handler *, uint64_t, void *);
int            reactor_timer_destruct(reactor_timer *);
int            reactor_timer_delete(reactor_timer *);
int            reactor_timer_interval(reactor_timer *, uint64_t);
void           reactor_timer_handler(reactor_event *);

#endif /* REACTOR_TIMER_H_INCLUDED */

