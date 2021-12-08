#ifndef REACTOR_DESCRIPTOR_H_INCLUDED
#define REACTOR_DESCRIPTOR_H_INCLUDED

#include "reactor.h"

enum descriptor_event_type
{
  DESCRIPTOR_READ,
  DESCRIPTOR_WRITE,
  DESCRIPTOR_CLOSE
};

typedef struct descriptor descriptor;
struct descriptor
{
  reactor_handler handler;
  reactor_handler epoll_handler;
  int             fd;
  int             write_notify;
};

void descriptor_construct(descriptor *, reactor_callback *, void *);
void descriptor_destruct(descriptor *);
void descriptor_open(descriptor *, int, int);
void descriptor_close(descriptor *);
void descriptor_write_notify(descriptor *, int);
int  descriptor_fd(descriptor *);

#endif /* REACTOR_DESCRIPTOR_H_INCLUDED */
