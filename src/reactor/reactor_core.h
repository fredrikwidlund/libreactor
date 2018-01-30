#ifndef REACTOR_CORE_H_INCLUDED
#define REACTOR_CORE_H_INCLUDED

#define REACTOR_CORE_MAX_EVENTS 32

enum
{
  REACTOR_ABORT = -1,
  REACTOR_ERROR = -1,
  REACTOR_OK    = 0
};

typedef struct reactor_descriptor reactor_descriptor;

int  reactor_core_construct(void);
void reactor_core_destruct(void);
int  reactor_core_run(void);
int  reactor_core_fd(void);
int  reactor_core_add(reactor_descriptor *, int);
void reactor_core_delete(reactor_descriptor *);
void reactor_core_modify(reactor_descriptor *, int);
void reactor_core_flush(reactor_descriptor *);
void reactor_core_enqueue(reactor_user_callback *, void *);

#endif /* REACTOR_CORE_H_INCLUDED */
