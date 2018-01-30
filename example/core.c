#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <netdb.h>

#include <dynamic.h>
#include <reactor.h>

struct state
{
  reactor_descriptor descriptor;
  int                foo;
};

int event(void *state, int type, void *data)
{
  struct state *s = state;
  int flags = *(int *) data;

  (void) type;
  if (flags & EPOLLERR)
    {
      reactor_descriptor_close(&s->descriptor);
      return REACTOR_ERROR;
    }

  (void) fprintf(stderr, "flags 0x%02x %d\n", *(int *) data, s->foo);
  reactor_descriptor_close(&s->descriptor);
  return REACTOR_OK;
}

int main()
{
  struct state s = {.foo = 42};
  int e;

  reactor_core_construct();
  e = reactor_descriptor_open(&s.descriptor, event, &s, 0, EPOLLIN | EPOLLET);
  if (e == REACTOR_ERROR)
    err(1, "reactor_descriptor_open");
  reactor_core_run();
  reactor_core_destruct();
}
