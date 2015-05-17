#ifndef REACTOR_SIGNAL_DISPATCHER_H_INCLUDED
#define REACTOR_SIGNAL_DISPATCHER_H_INCLUDED

#define REACTOR_SIGNAL_DISPATCHER_RAISED 1
#define REACTOR_SIGNAL_DISPATCHER_SIGNO  SIGUSR1

typedef struct reactor_signal_dispatcher reactor_signal_dispatcher;

struct reactor_signal_dispatcher
{
  reactor            *reactor;
  reactor_signal      signal;
  int                 ref;
};

reactor_signal_dispatcher *reactor_signal_dispatcher_new(reactor *);
int                        reactor_signal_dispatcher_construct(reactor *, reactor_signal_dispatcher *);
int                        reactor_signal_dispatcher_destruct(reactor_signal_dispatcher *);
int                        reactor_signal_dispatcher_delete(reactor_signal_dispatcher *);
void                       reactor_signal_dispatcher_handler(reactor_event *);

#endif /* REACTOR_SIGNAL_DISPATCHER_H_INCLUDED */

