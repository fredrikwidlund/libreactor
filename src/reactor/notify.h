#ifndef REACTOR_NOFITY_H_INCLUDED
#define REACTOR_NOTIFY_H_INCLUDED

enum notify_event_type
{
  NOTIFY_EVENT
};

typedef struct notify_event notify_event;
typedef struct notify_entry notify_entry;
typedef struct notify       notify;

struct notify_event
{
  int              id;
  char            *path;
  char            *name;
  uint32_t         mask;
};

struct notify_entry
{
  int              id;
  char            *path;
};

struct notify
{
  reactor_handler  handler;
  descriptor       descriptor;
  list             entries;
  size_t           errors;
  int             *cancel;
};

void   notify_construct(notify *, reactor_callback *, void *);
void   notify_destruct(notify *);
int    notify_watch(notify *, char *, uint32_t);
void   notify_clear(notify *);

#endif /* REACTOR_NOTIFY_H_INCLUDED */
