#ifndef REACTOR_CORE_H_INCLUDED
#define REACTOR_CORE_H_INCLUDED

enum reactor_core_state
{
  REACTOR_CORE_CLOSED = 0,
  REACTOR_CORE_OPEN,
  REACTOR_CORE_RUNNING
};

typedef struct reactor_core reactor_core;
struct reactor_core
{
  int    state;
  int    current;
  vector polls;
  vector descs;
};

int  reactor_core_open(void);
int  reactor_core_run(void);
void reactor_core_close(void);
int  reactor_core_desc_add(reactor_desc *, int, int);
void reactor_core_desc_remove(reactor_desc *);
void reactor_core_desc_events(reactor_desc *, int);
void reactor_core_desc_set(reactor_desc *, int);
void reactor_core_desc_clear(reactor_desc *, int);
int  reactor_core_desc_fd(reactor_desc *);

#endif /* REACTOR_CORE_H_INCLUDED */
