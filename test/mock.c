#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <cmocka.h>

int debug_abort = 0;

void __real_abort(void);
void __wrap_abort(void)
{
  if (debug_abort)
    mock_assert(0, "", "", 0);
  else
    __real_abort();
}
