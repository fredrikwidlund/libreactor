#ifndef REACTOR_SIGNAL_H_INCLUDED
#define REACTOR_SIGNAL_H_INCLUDED

#define REACTOR_SIGNAL_RAISED 1

typedef struct reactor_signal reactor_signal;

struct reactor_signal
{
  reactor_call        user;
  reactor            *reactor;
  int                 descriptor;
  struct epoll_event  ev;
  reactor_call        main;
};

reactor_signal *reactor_signal_new(reactor *, int, void *, reactor_handler *, void *);
int             reactor_signal_construct(reactor *, reactor_signal *, int, void *, reactor_handler *, void *);
int             reactor_signal_destruct(reactor_signal *);
int             reactor_signal_delete(reactor_signal *);
void            reactor_signal_handler(reactor_event *);

#endif /* REACTOR_SIGNAL_H_INCLUDED */
