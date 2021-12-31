#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <setjmp.h>
#include <cmocka.h>

#include "reactor.h"

static void set(void *p1, void *p2)
{
  uint32_t *a = p1, *b = p2;

  *a = b ? *b : 0;
}

static int equal(void *p1, void *p2)
{
  uint32_t *a = p1, *b = p2;

  return b ? *a == *b : *a == 0;
}

static size_t hash(void *p)
{
  return *(uint32_t *) p;
}

void test_map(__attribute__((unused)) void **state)
{
  map m;
  uint32_t last = MAP_ELEMENTS_CAPACITY_MIN - 1, coll1 = 1 << 16, coll2 = 1 << 17;

  map_construct(&m, sizeof(uint32_t), set);

  /* erase non-existing object */
  map_erase(&m, (uint32_t[]) {42}, hash, set, equal, NULL);
  assert_int_equal(map_size(&m), 0);

  /* erase object with duplicate */
  map_insert(&m, (uint32_t[]) {1}, hash, set, equal, NULL);
  map_insert(&m, (uint32_t[]) {1 + coll1}, hash, set, equal, NULL);
  map_erase(&m, (uint32_t[]) {1}, hash, set, equal, NULL);
  assert_int_equal(map_size(&m), 1);

  /* erase object with empty succ */
  map_clear(&m, set, NULL, NULL);
  map_insert(&m, (uint32_t[]) {1}, hash, set, equal, NULL);
  map_erase(&m, (uint32_t[]) {5}, hash, set, equal, NULL);
  assert_false(equal(map_at(&m, (uint32_t[]) {1}, hash, equal), NULL));

  /* erase when w wraps */
  map_clear(&m, set, NULL, NULL);
  map_insert(&m, (uint32_t[]) {last + coll1}, hash, set, equal, NULL);
  map_insert(&m, (uint32_t[]) {last}, hash, set, equal, NULL);
  map_insert(&m, (uint32_t[]) {last + coll2}, hash, set, equal, NULL);
  map_erase(&m, (uint32_t[]) {last}, hash, set, equal, NULL);
  assert_false(equal(map_at(&m, (uint32_t[]) {last + coll1}, hash, equal), NULL));
  assert_false(equal(map_at(&m, (uint32_t[]) {last + coll2}, hash, equal), NULL));

  /* erase when i wraps */
  map_clear(&m, set, NULL, NULL);
  map_insert(&m, (uint32_t[]) {last}, hash, set, equal, NULL);
  map_erase(&m, (uint32_t[]) {last}, hash, set, equal, NULL);
  assert_true(equal(map_at(&m, (uint32_t[]) {last}, hash, equal), NULL));

  /* erase when i wraps  and w < i */
  map_clear(&m, set, NULL, NULL);
  map_insert(&m, (uint32_t[]) {last}, hash, set, equal, NULL);
  map_insert(&m, (uint32_t[]) {coll1}, hash, set, equal, NULL);
  map_insert(&m, (uint32_t[]) {last + coll1}, hash, set, equal, NULL);
  map_erase(&m, (uint32_t[]) {last}, hash, set, equal, NULL);
  assert_false(equal(map_at(&m, (uint32_t[]) {coll1}, hash, equal), NULL));
  assert_false(equal(map_at(&m, (uint32_t[]) {last + coll1}, hash, equal), NULL));
  assert_true(equal(map_at(&m, (uint32_t[]) {last}, hash, equal), NULL));

  /* erase when i wraps and w > o */
  map_clear(&m, set, NULL, NULL);
  map_insert(&m, (uint32_t[]) {last - 1}, hash, set, equal, NULL);
  map_insert(&m, (uint32_t[]) {last}, hash, set, equal, NULL);
  map_insert(&m, (uint32_t[]) {last + coll1}, hash, set, equal, NULL);
  map_erase(&m, (uint32_t[]) {last - 1}, hash, set, equal, NULL);
  assert_false(equal(map_at(&m, (uint32_t[]) {last}, hash, equal), NULL));
  assert_false(equal(map_at(&m, (uint32_t[]) {last + coll1}, hash, equal), NULL));
  assert_true(equal(map_at(&m, (uint32_t[]) {last - 1}, hash, equal), NULL));

  /* erase when j wraps */
  map_clear(&m, set, NULL, NULL);
  map_insert(&m, (uint32_t[]) {last}, hash, set, equal, NULL);
  map_insert(&m, (uint32_t[]) {coll1}, hash, set, equal, NULL);
  map_insert(&m, (uint32_t[]) {coll2}, hash, set, equal, NULL);
  map_erase(&m, (uint32_t[]) {last}, hash, set, equal, NULL);
  assert_false(equal(map_at(&m, (uint32_t[]) {coll1}, hash, equal), NULL));
  assert_false(equal(map_at(&m, (uint32_t[]) {coll2}, hash, equal), NULL));
  assert_true(equal(map_at(&m, (uint32_t[]) {last}, hash, equal), NULL));

  map_destruct(&m, NULL, NULL);
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(test_map)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
