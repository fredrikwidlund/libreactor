#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <setjmp.h>
#include <cmocka.h>

#include "reactor.h"

static void release(maps_entry *e)
{
  free(e->key);
}

void test_maps(__attribute__((unused)) void **state)
{
  maps maps;
  int i;
  void *p;
  char key[16];

  maps_construct(&maps);

  for (i = 0; i < 2; i ++)
  {
    p = malloc(5);
    strcpy(p, "test");
    maps_insert(&maps, p, 42, release);
  }
  assert_true(maps_at(&maps, "test") == 42);
  assert_true(maps_size(&maps) == 1);
  maps_erase(&maps, "test2", release);
  assert_true(maps_size(&maps) == 1);
  maps_erase(&maps, "test", release);
  assert_true(maps_size(&maps) == 0);

  maps_reserve(&maps, 32);
  assert_true(maps.map.elements_capacity == 64);

  for (i = 0; i < 100; i++)
  {
    snprintf(key, sizeof key, "test%d", i);
    p = malloc(strlen(key) + 1);
    strcpy(p, key);
    maps_insert(&maps, p, i, release);
  }
  maps_clear(&maps, release);
  maps_destruct(&maps, release);
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(test_maps)
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
