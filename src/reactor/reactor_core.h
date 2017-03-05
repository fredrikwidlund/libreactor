#ifndef REACTOR_CORE_H_INCLUDED
#define REACTOR_CORE_H_INCLUDED

enum reactor_core_event
{
  REACTOR_CORE_EVENT_FD
};

void  reactor_core_construct();
int   reactor_core_run();
void  reactor_core_destruct();
void  reactor_core_fd_register(int, reactor_user_callback *, void *, int);
void  reactor_core_fd_deregister(int);
void *reactor_core_fd_poll(int);
void *reactor_core_fd_user(int);
void  reactor_core_job_register(reactor_user_callback *, void *);

#endif /* REACTOR_CORE_H_INCLUDED */
