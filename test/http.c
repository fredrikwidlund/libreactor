#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

static void basic(__attribute__ ((unused)) void **unused)
{
}

int main()
{
  const struct CMUnitTest tests[] =
    {
     cmocka_unit_test(basic)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
