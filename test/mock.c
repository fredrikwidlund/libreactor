#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/epoll.h>

#include <cmocka.h>

int debug_epoll_wait = 0;

int __real_epoll_wait(int, struct epoll_event *, int, int);
int __wrap_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
  if (debug_epoll_wait)
  {
    errno = EINTR;
    return -1;
  }
  else
    return __real_epoll_wait(epfd, events, maxevents, timeout);
}
