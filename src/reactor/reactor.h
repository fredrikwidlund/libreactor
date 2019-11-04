#ifndef REACTOR_REACTOR_H_INCLUDED
#define REACTOR_REACTOR_H_INCLUDED

enum reactor_status
{
 REACTOR_ABORT = -2,
 REACTOR_ERROR = -1,
 REACTOR_OK    = 0
};

typedef uintptr_t            reactor_id;
typedef enum reactor_status  reactor_status;
typedef struct reactor_event reactor_event;

struct reactor_event
{
  void      *state;
  int        type;
  uintptr_t  data;
};

void reactor_construct(void);
void reactor_destruct(void);
void reactor_run(void);

#endif /* REACTOR_REACTOR_H_INCLUDED */
