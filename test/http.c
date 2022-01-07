#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cmocka.h>
#include "reactor.h"

static void test_request(data method, data target, data host, data type, data body)
{
  stream s;
  ssize_t n;
  data m, t;
  http_field f[16];
  size_t c;

  stream_construct(&s, NULL, NULL);
  http_write_request(&s, method, target, host, type, body);
  buffer_insert(&s.input, 0, s.output.data, s.output.size);
  c = 16;
  n = http_read_request(&s, &m, &t, f, &c);
  assert_true(n > 0);
  assert_true(data_equal(method, m));
  assert_true(data_equal(target, t));
  assert_true(data_equal(host, http_field_lookup(f, c, data_string("Host"))));
  assert_true(data_equal(type, http_field_lookup(f, c, data_string("Content-Type"))));
  stream_consume(&s, n);
  assert_true(s.input.size == body.size && memcmp(s.input.data, body.base, body.size) == 0);
  stream_destruct(&s);
}

static void test_response(data status, data date, data type, data body)
{
  stream s;
  ssize_t n;
  data status_read;
  http_field f[16];
  size_t c;
  int code;
  
  stream_construct(&s, NULL, NULL);
  http_write_response(&s, status, date, type, body);
  buffer_insert(&s.input, 0, s.output.data, s.output.size);
  c = 16;
  n = http_read_response(&s, &code, &status_read, f, &c);
  assert_true(n > 0);
  assert_true(data_equal(date, http_field_lookup(f, c, data_string("Date"))));
  assert_true(data_equal(type, http_field_lookup(f, c, data_string("Content-Type"))));
  stream_consume(&s, n);
  assert_true(s.input.size == body.size && memcmp(s.input.data, body.base, body.size) == 0);

  assert_true(data_equal(data_null(), http_field_lookup(f, c, data_string("Datx"))));
  assert_true(data_equal(data_null(), http_field_lookup(f, c, data_string("Some-Unused-Name"))));
  stream_destruct(&s);
}

static void test_http(__attribute__((unused)) void **state)
{
  test_request(data_string("GET"), data_string("/"), data_string("some.host"), data_null(), data_null());
  test_request(data_string("POST"), data_string("/"), data_string("some.host"), data_string("text/plain"), data_string("test"));
  test_response(data_string("200 OK"), data_string("Wed, 05 Jan 2022 14:12:35 GMT"), data_null(), data_null());
  test_response(data_string("200 OK"), data_string("Wed, 05 Jan 2022 14:12:35 GMT"), data_string("text/plain"), data_string("test"));
}

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_http)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
