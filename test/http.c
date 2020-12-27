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
#include <sys/stat.h>
#include <dirent.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

static void date(__attribute__((unused)) void **unused)
{
  // update and get date
  (void) http_date(1);
  (void) http_date(0);
}

static void headers(__attribute__((unused)) void **unused)
{
  http_headers headers;
  int i;

  http_headers_construct(&headers);
  http_headers_add(&headers, segment_string("Name"), segment_string("Value"));
  assert_int_equal(http_headers_count(&headers), 1);
  assert_true(segment_equal(http_headers_lookup(&headers, segment_string("Name")), segment_string("Value")));
  assert_true(segment_equal(http_headers_lookup(&headers, segment_string("Fail")), segment_empty()));
  for (i = 0; i < 128; i++)
    http_headers_add(&headers, segment_string("Name"), segment_string("Value"));
}

static segment iterate_read(char *file)
{
  int fd;
  struct stat st;
  char *data;
  ssize_t n;

  fd = open(file, O_RDONLY);
  assert_true(fd >= 0);
  assert_int_equal(fstat(fd, &st), 0);
  data = malloc(st.st_size);
  n = read(fd, data, st.st_size);
  assert_int_equal(n, st.st_size);
  return segment_data(data, st.st_size);
}

static void iterate_request(char *file, int allow)
{
  http_request request;
  segment s;
  ssize_t n;
  size_t total = 0;

  fprintf(stderr, "<%s>\n", file);
  s = iterate_read(file);
  while (1)
  {
    n = http_request_read(&request, segment_offset(s, total));
    if (n <= 0)
      break;
    total += n;
  }
  if (allow)
    assert_int_equal(total, s.size);
  else
    assert_int_not_equal(total, s.size);
  free(s.base);
}

static void iterate_response(char *file, int allow)
{
  http_response response;
  segment s;
  ssize_t n;
  size_t total = 0;

  fprintf(stderr, "<%s>\n", file);
  s = iterate_read(file);
  while (1)
  {
    n = http_response_read(&response, segment_offset(s, total));
    if (n <= 0)
      break;
    total += n;
  }
  if (allow)
    assert_int_equal(total, s.size);
  else
    assert_int_not_equal(total, s.size);
  free(s.base);
}

static void iterate(__attribute__((unused)) void **unused)
{
  char cwd[PATH_MAX];
  DIR *dir;
  struct dirent *d;
  int request, allow;

  getcwd(cwd, sizeof cwd);
  assert_int_equal(chdir(SRCDIR "test/data/"), 0);
  dir = opendir(".");
  assert_true(dir != NULL);
  while (1)
  {
    d = readdir(dir);
    if (!d)
      break;
    if (d->d_type != DT_REG || strlen(d->d_name) < 5)
      continue;
    request = strncmp(d->d_name, "request", strlen("request")) == 0;
    allow = strcmp(d->d_name + strlen(d->d_name) - 5, "allow") == 0;
    if (request)
      iterate_request(d->d_name, allow);
    else
      iterate_response(d->d_name, allow);
  }
  (void) closedir(dir);
  assert_int_equal(chdir(cwd), 0);
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(date),
          cmocka_unit_test(headers),
          cmocka_unit_test(iterate)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
