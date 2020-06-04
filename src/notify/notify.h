#ifndef NOFITY_H_INCLUDED
#define NOTIFY_H_INCLUDED

enum
{
  NOTIFY_ERROR,
  NOTIFY_EVENT
};

typedef struct notify notify;
struct notify
{
  core_handler user;
  int          fd;
  int          next;
  int          error;
};

void notify_construct(notify *, core_callback *, void *);
void notify_destruct(notify *);
void notify_watch(notify *, char *, uint32_t);
int  notify_valid(notify *);

#endif /* NOTIFY_H_INCLUDED */
