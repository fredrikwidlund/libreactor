#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>

int debug_signal_error = 0;

int __real_sigprocmask(int, const sigset_t *restrict, sigset_t *restrict);
int __wrap_sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset)
{
  return debug_signal_error == 1 ? -1 : __real_sigprocmask(how, set, oset);
}

int __real_signalfd(int, const sigset_t *, int);
int __wrap_signalfd(int fd, const sigset_t *mask, int flags)
{
  if (debug_signal_error == 2)
    return -1;

  if (debug_signal_error == 3)
    return INT_MAX;

  return __real_signalfd(fd, mask, flags);
}

int debug_io_error = 0;

int __real_fcntl(int, int, int);
int __wrap_fcntl(int fd, int cmd, int arg)
{
  return debug_io_error ? -1 : __real_fcntl(fd, cmd, arg);
}

int __real_timerfd_create(int, int);
int __wrap_timerfd_create(int id, int flags)
{
  return debug_io_error ? -1 : __real_timerfd_create(id, flags);
}

ssize_t __real_read(int, void *, size_t);
ssize_t __wrap_read(int fd, void *buf, size_t nbyte)
{
  if (debug_io_error)
    {
      errno = 255;
      return -1;
    }

  return __real_read(fd, buf, nbyte);
}

int debug_epoll_error = 0;

int __real_epoll_create1(int);
int __wrap_epoll_create1(int flags)
{
  return debug_epoll_error ? -1 : __real_epoll_create1(flags);
}

int __real_epoll_wait(int, struct epoll_event *, int, int);
int __wrap_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
  return debug_epoll_error ? -1 : __real_epoll_wait(epfd, events, maxevents, timeout);
}

int __real_epoll_ctl(int, int, int, struct epoll_event *);
int __wrap_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
  return debug_epoll_error ? -1 : __real_epoll_ctl(epfd, op, fd, event);
}

int debug_out_of_memory = 0;

void *__real_malloc(size_t);
void *__wrap_malloc(size_t size)
{
  return debug_out_of_memory ? NULL : __real_malloc(size);
}

void *__real_calloc(size_t, size_t);
void *__wrap_calloc(size_t n, size_t size)
{
  return debug_out_of_memory ? NULL : __real_calloc(n, size);
}

void *__real_realloc(void *, size_t);
void *__wrap_realloc(void *p, size_t size)
{
  return debug_out_of_memory ? NULL : __real_realloc(p, size);
}

void *__real_aligned_alloc(size_t, size_t);
void *__wrap_aligned_alloc(size_t align, size_t size)
{
  return debug_out_of_memory ? NULL : __real_aligned_alloc(align, size);
}
