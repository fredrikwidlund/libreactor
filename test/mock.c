#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <setjmp.h>
#include <cmocka.h>

int mock_poll_failure = 0;

int __real_poll(struct pollfd *, nfds_t, int);
int __wrap_poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
  return mock_poll_failure ? -1 : __real_poll(fds, nfds, timeout);
}
