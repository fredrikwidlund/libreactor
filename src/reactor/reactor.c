#define _GNU_SOURCE

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <dynamic.h>

#include "reactor.h"

struct reactor
{
  size_t ref;
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
    pool_construct(NULL, NULL);
  }
  reactor.ref++;
}

void reactor_destruct(void)
{
  reactor.ref--;
  if (!reactor.ref)
  {
    pool_destruct(NULL);
    core_destruct(NULL);
  }
}

void reactor_loop(void)
{
  core_loop(NULL);
}

int reactor_clone(int n)
{
  int i;
  pid_t cpid;

  for (i = 0; i < n; i++)
  {
    cpid = fork();
    if (!cpid)
      return i;
  }

  wait(NULL);
  exit(0);
}

int reactor_affinity(int cpu)
{
  cpu_set_t set;

  CPU_ZERO(&set);
  CPU_SET(cpu, &set);
  return sched_setaffinity(0, sizeof set, &set);
}
