#ifndef REACTOR_NOFITY_H_INCLUDED
#define REACTOR_NOTIFY_H_INCLUDED

enum
{
  NOTIFY_ERROR = 0,
  NOTIFY_EVENT = 1
};

typedef struct notify notify;
struct notify
{
  int          state;
  core_handler user;
  int          fd;
  int          next;
  int          error;
};

void notify_construct(notify *, core_callback *, void *);
void notify_destruct(notify *);
void notify_watch(notify *, char *, uint32_t);
int  notify_error(notify *);

#endif /* NOTIFY_H_INCLUDED */
