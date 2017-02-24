#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <setjmp.h>
#include <errno.h>

#include <cmocka.h>

int mock_poll_failure = 0;
int __real_poll(struct pollfd *, nfds_t, int);
int __wrap_poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
  return mock_poll_failure ? -1 : __real_poll(fds, nfds, timeout);
}

int mock_read_failure = 0;
ssize_t __real_read(int, void *, size_t);
ssize_t __wrap_read(int fildes, void *buf, size_t nbyte)
{
  if (!mock_read_failure)
    return __real_read(fildes, buf, nbyte);

  errno = mock_read_failure;
  mock_read_failure = 0;
  return -1;
}
