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

void notify_construct(notify *, core_callback *, void *, char *, uint32_t);
void notify_destruct(notify *);
int  notify_error(notify *);

#endif /* NOTIFY_H_INCLUDED */
