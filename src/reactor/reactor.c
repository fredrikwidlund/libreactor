#include <signal.h>

#include <dynamic.h>

#include "reactor.h"

struct reactor
{
  size_t  ref;
};

static __thread struct reactor reactor = {0};

static void reactor_abort(__attribute__((unused)) int arg)
{
  core_abort(NULL);
}

void reactor_construct(void)
{
  if (!reactor.ref)
  {
    signal(SIGTERM, reactor_abort);
    signal(SIGINT, reactor_abort);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    core_construct(NULL);
  }
  reactor.ref++;
}

void reactor_destruct(void)
{
  reactor.ref--;
  if (!reactor.ref)
    core_destruct(NULL);
}

void reactor_loop(void)
{
  core_loop(NULL);
}
