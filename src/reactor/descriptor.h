#ifndef REACTOR_DESCRIPTOR_H_INCLUDED
#define REACTOR_DESCRIPTOR_H_INCLUDED

#include "core.h"

enum descriptor_mask
{
  DESCRIPTOR_CLOSE = 0,
  DESCRIPTOR_READ  = 1,
  DESCRIPTOR_WRITE = 2,
  DESCRIPTOR_LEVEL = 4
};

typedef struct descriptor descriptor;

struct descriptor
{
  reactor_handler      handler;
  reactor_handler      epoll_handler;
  int                  fd;
  enum descriptor_mask mask;
};

void descriptor_construct(descriptor *, reactor_callback *, void *);
void descriptor_destruct(descriptor *);
void descriptor_open(descriptor *, int, enum descriptor_mask);
void descriptor_mask(descriptor *, enum descriptor_mask);
void descriptor_close(descriptor *);
int  descriptor_fd(descriptor *);
int  descriptor_active(descriptor *);

#endif /* REACTOR_DESCRIPTOR_H_INCLUDED */
