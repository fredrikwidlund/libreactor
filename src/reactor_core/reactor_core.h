#ifndef REACTOR_CORE_H_INCLUDED
#define REACTOR_CORE_H_INCLUDED

#ifndef REACTOR_CORE_QUEUE_SIZE
#define REACTOR_CORE_QUEUE_SIZE 1024
#endif

enum REACTOR_CORE_EVENTS
{
  REACTOR_CORE_ERROR = 0x00,
  REACTOR_CORE_CALL  = 0x01
};

typedef struct reactor_core reactor_core;
struct reactor_core
{
  int      fd;
  int      ref;
  vector   calls;
};

int  reactor_core_construct(void);
int  reactor_core_mask_to_epoll(int);
int  reactor_core_mask_from_epoll(int);
int  reactor_core_add(int, int, reactor_desc *);
int  reactor_core_mod(int, int, reactor_desc *);
int  reactor_core_del(int);
int  reactor_core_run(void);
void reactor_core_schedule(reactor_user_call *, void *);
void reactor_core_destruct(void);

#endif /* REACTOR_CORE_H_INCLUDED */
