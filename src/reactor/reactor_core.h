#ifndef REACTOR_CORE_H_INCLUDED
#define REACTOR_CORE_H_INCLUDED

enum reactor_core_event
{
  REACTOR_CORE_EVENT_POLL
};

void  reactor_core_construct();
void  reactor_core_register(int, reactor_user_callback *, void *, int);
void  reactor_core_deregister(int);
void *reactor_core_poll(int);
void *reactor_core_user(int);
int   reactor_core_run();
void  reactor_core_destruct();

#endif /* REACTOR_CORE_H_INCLUDED */
