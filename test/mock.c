#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <errno.h>
#include <cmocka.h>

int debug_abort = 0;
int debug_read = 0;
int debug_inotify_init1 = 0;

void __real_abort(void);
void __wrap_abort(void)
{
  if (debug_abort)
    mock_assert(0, "", "", 0);
  else
    __real_abort();
}

ssize_t __real_read(int, void *, size_t);
ssize_t __wrap_read(int fildes, void *buf, size_t nbyte)
{
  if (debug_read)
  {
    errno = debug_read;
    return -1;
  }
  else
    return __real_read(fildes, buf, nbyte);
}

ssize_t __real_inotify_init1(int);
ssize_t __wrap_inotify_init1(int flags)
{
  return debug_inotify_init1 ? -1 : __real_inotify_init1(flags);
}
