#ifndef REACTOR_EVENT_H_INCLUDED
#define REACTOR_EVENT_H_INCLUDED

#include "core.h"
#include "descriptor.h"

enum
{
  EVENT_SIGNAL
};

typedef struct event event;

struct event
{
  reactor_handler handler;
  descriptor      descriptor;
};

void event_construct(event *, reactor_callback *, void *);
void event_destruct(event *);
void event_open(event *);
int  event_is_open(event *);
void event_signal(event *);
void event_close(event *);


#endif /* REACTOR_EVENT_H_INCLUDED */
