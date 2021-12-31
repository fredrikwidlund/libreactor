#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <setjmp.h>
#include <cmocka.h>

#include "reactor.h"

static void release(void *object)
{
  printf("free %p\n", *(void **) object);
  free(*(void **) object);
  printf("freed\n");
}

static int compare(void *a, void *b)
{
  char *sa = *(char **) a, *sb = *(char **) b;

  return strcmp(sa, sb);
}

void test_core(__attribute__((unused)) void **state)
{
  list l;
  char *a[] = {"a", "list", "of", "string", "pointers"}, **s;
  size_t i;

  list_construct(&l);
  for (i = 0; i < sizeof a / sizeof *a; i++)
    list_push_back(&l, &a[i], sizeof(char *));

  s = list_front(&l);
  assert_string_equal(*s, "a");

  list_push_front(&l, (char *[]) {"test"}, sizeof(char *));
  s = list_front(&l);
  assert_string_equal(*s, "test");
  list_erase(s, NULL);

  i = 0;
  list_foreach(&l, s)
      assert_string_equal(*s, a[i++]);

  i = sizeof a / sizeof *a;
  list_foreach_reverse(&l, s)
      assert_string_equal(*s, a[--i]);

  s = list_find(&l, compare, (char *[]) {"pointers"});
  assert_string_equal(*s, "pointers");

  s = list_find(&l, compare, (char *[]) {"foo"});
  assert_true(!s);

  list_destruct(&l, NULL);
}

void test_list_release(__attribute__((unused)) void **state)
{
  list l;
  void *p;
  int i;

  list_construct(&l);
  for (i = 0; i < 16; i++)
  {
    p = malloc(4096);
    list_push_back(&l, &p, sizeof p);
  }
  list_destruct(&l, release);
}

void test_unit(__attribute__((unused)) void **state)
{
  list l, l2;
  int *p;

  list_construct(&l);

  list_insert(list_front(&l), (int[]) {1}, sizeof(int));
  assert_int_equal(*(int *) list_front(&l), 1);
  list_clear(&l, NULL);

  p = list_insert(list_front(&l), NULL, sizeof(int));
  *p = 42;
  assert_int_equal(*(int *) list_front(&l), 42);
  list_clear(&l, NULL);

  list_insert(list_previous(list_front(&l)), (int[]) {1}, sizeof(int));
  assert_int_equal(*(int *) list_front(&l), 1);
  list_erase(list_back(&l), NULL);

  list_push_front(&l, (int[]) {1}, sizeof(int));
  assert_int_equal(*(int *) list_front(&l), 1);
  list_erase(list_front(&l), NULL);

  list_push_back(&l, (int[]) {1}, sizeof(int));
  assert_int_equal(*(int *) list_front(&l), 1);
  list_clear(&l, NULL);

  list_push_back(&l, (int[]) {1}, sizeof(int));
  list_push_back(&l, (int[]) {2}, sizeof(int));
  list_push_back(&l, (int[]) {3}, sizeof(int));
  p = list_next(list_front(&l));
  assert_int_equal(*p, 2);
  list_erase(p, NULL);
  p = list_next(list_front(&l));
  assert_int_equal(*p, 3);

  list_clear(&l, NULL);
  list_construct(&l2);
  list_push_back(&l, (int[]) {1}, sizeof(int));
  list_push_back(&l, (int[]) {2}, sizeof(int));
  list_push_back(&l, (int[]) {3}, sizeof(int));
  assert_true(list_empty(&l2));
  list_splice(list_front(&l2), list_next(list_front(&l)));
  assert_int_equal(*(int *) list_front(&l2), 2);
  assert_int_equal(*(int *) list_next(list_front(&l)), 3);
  list_destruct(&l2, NULL);

  list_destruct(&l, NULL);
}

void test_edge(__attribute__((unused)) void **state)
{
  list l1;
  int *i;

  list_construct(&l1);

  i = list_push_back(&l1, (int[]) {1}, sizeof(int));
  list_splice(i, i);
  list_destruct(&l1, NULL);
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_list_release),
    cmocka_unit_test(test_core),
    cmocka_unit_test(test_unit),
    cmocka_unit_test(test_edge)
  };
  
  return cmocka_run_group_tests(tests, NULL, NULL);
}
