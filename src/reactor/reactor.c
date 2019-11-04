#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "../reactor.h"

void reactor_construct(void)
{
  reactor_core_construct();
  reactor_pool_construct();
  reactor_resolver_construct();
}

void reactor_destruct(void)
{
  reactor_resolver_destruct();
  reactor_pool_destruct();
  reactor_core_destruct();
}

void reactor_run(void)
{
  reactor_core_run();
}
