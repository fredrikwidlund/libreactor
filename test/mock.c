#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <errno.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cmocka.h>

int mock_epoll_wait_failure = 0;
int mock_epoll_create_failure = 0;
int mock_epoll_ctl_failure = 0;
int mock_socketpair_failure = 0;
int mock_out_of_memory = 0;
int mock_abort = 0;
int mock_read_failure = 0;
int mock_write_failure = 0;
int mock_accept_failure = 0;
int mock_bind_failure = 0;
int mock_listen_failure = 0;
int mock_connect = 0, mock_connect_errno = 0, mock_connect_return = 0;
int mock_socket_failure = 0;
int mock_timerfd_failure = 0;

int __real_epoll_wait(int, struct epoll_event *, int, int);
int __wrap_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
  return mock_epoll_wait_failure ? -1 : __real_epoll_wait(epfd, events, maxevents, timeout);
}

int __real_epoll_pwait(int, struct epoll_event *, int, int, const sigset_t *);
int __wrap_epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout, const sigset_t *sigmask)
{
  return mock_epoll_wait_failure ? -1 : __real_epoll_pwait(epfd, events, maxevents, timeout, sigmask);
}

int __real_epoll_create1(int);
int __wrap_epoll_create1(int flags)
{
  return mock_epoll_create_failure ? -1 : __real_epoll_create1(flags);
}

int __real_epoll_ctl(int, int, int, struct epoll_event *);
int __wrap_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
  return mock_epoll_ctl_failure ? -1 : __real_epoll_ctl(epfd, op, fd, event);
}

int __real_socketpair(int, int, int, int[2]);
int __wrap_socketpair(int domain, int type, int protocol, int socket_vector[2])
{
  return mock_socketpair_failure ? -1 : __real_socketpair(domain, type, protocol, socket_vector);
}

void *__real_malloc(size_t);
void *__wrap_malloc(size_t size)
{
  return mock_out_of_memory ? NULL : __real_malloc(size);
}

void __real_abort(void);
void __wrap_abort(void)
{
  if (mock_abort)
    mock_assert(0, "", "", 0);
  else
    __real_abort();
}

ssize_t __real_read(int, void *, size_t);
ssize_t __wrap_read(int fildes, void *buf, size_t nbyte)
{
  if (!mock_read_failure)
    return __real_read(fildes, buf, nbyte);

  errno = mock_read_failure;
  mock_read_failure = 0;
  return -1;
}

ssize_t __real_write(int, void *, size_t);
ssize_t __wrap_write(int fildes, void *buf, size_t nbyte)
{
  if (!mock_write_failure)
    return __real_write(fildes, buf, nbyte);

  errno = mock_write_failure;
  mock_write_failure = 0;
  return -1;
}

int __real_accept(int, struct sockaddr *restrict,
                  socklen_t *restrict);
int __wrap_accept(int socket, struct sockaddr *restrict address,
                  socklen_t *restrict address_len)
{
  if (!mock_accept_failure)
    return __real_accept(socket, address, address_len);

  errno = mock_accept_failure;
  mock_accept_failure = 0;
  return -1;
}

int __real_bind(int, const struct sockaddr *,
           socklen_t);
int __wrap_bind(int socket, const struct sockaddr *address,
           socklen_t address_len)
{
  if (!mock_bind_failure)
    return __real_bind(socket, address, address_len);

  errno = mock_bind_failure;
  mock_bind_failure = 0;
  return -1;
}

int __real_listen(int, int);
int __wrap_listen(int socket, int backlog)
{
  if (!mock_listen_failure)
    return __real_listen(socket, backlog);

  errno = mock_listen_failure;
  mock_listen_failure = 0;
  return -1;
}

int __real_connect(int, const struct sockaddr *,
           socklen_t);
int __wrap_connect(int socket, const struct sockaddr *address,
           socklen_t address_len)
{
  if (!mock_connect)
    return __real_connect(socket, address, address_len);

  mock_connect = 0;
  errno = mock_connect_errno;
  return mock_connect_return;
}

int __real_socket(int, int, int);
int __wrap_socket(int domain, int type, int protocol)
{
  if (!mock_socket_failure)
    return __real_socket(domain, type, protocol);

  errno = mock_socket_failure;
  mock_socket_failure = 0;
  return -1;
}

int __real_timerfd_create(int, int);
int __wrap_timerfd_create(int clockid, int flags)
{
  if (!mock_timerfd_failure)
    return __real_timerfd_create(clockid, flags);
  errno = mock_write_failure;
  mock_timerfd_failure = 0;
  return -1;
}

int __real_timerfd_settime(int, int, const struct itimerspec *, struct itimerspec *);
int __wrap_timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value)
{
  if (!mock_timerfd_failure)
    return __real_timerfd_settime(fd, flags, new_value, old_value);
  mock_timerfd_failure = 0;
  return -1;
}
