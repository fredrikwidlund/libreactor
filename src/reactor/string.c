#define _GNU_SOURCE

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "reactor.h"
#include "data.h"
#include "string.h"

struct string_header
{
  size_t     size;
  size_t     capacity;
  char       string[1];
};

static struct string_header string_header_null_object = {0};
static string string_null_object = string_header_null_object.string;

static struct string_header *string_header(string s)
{
  return (struct string_header *) ((uintptr_t) s - offsetof(struct string_header, string));
}

/* allocators */

string string_new(void)
{
  return string_null_object;
}

void string_free(string s)
{
  if (!string_null(s))
    free(string_header(s));
}

string string_copy(string s)
{
  return string_empty(s) ? string_null_object : string_append_data(string_new(), string_data(s));
}

string string_read(int fd)
{
  string s;
  char buffer[4096];
  ssize_t n;

  s = string_new();
  while (1)
  {
    n = read(fd, buffer, sizeof buffer);
    assert(n >= 0);
    if (!n)
      break;
    s = string_append_data(s, data_construct(buffer, n));
  }
  return s;
}

string string_load(const char *path)
{
  string s;
  int fd;

  fd = open(path, O_RDONLY);
  if (fd == -1)
    return string_null_object;
  s = string_read(fd);
  close(fd);
  return s;
}

/* capacity */

string string_allocate(string s, size_t capacity)
{
  struct string_header *header = string_header(s);

  if (capacity >= header->capacity)
  {
    header = string_null(s) ? calloc(1, sizeof *header + capacity) : realloc(header, sizeof *header + capacity);
    header->capacity = capacity;
    s = header->string;
  }
  return s;
}

string string_resize(string s, size_t size)
{
  s = string_allocate(s, size);
  string_header(s)->size = size;
  s[size] = 0;
  return s;
}

size_t string_size(string s)
{
  return string_header(s)->size;
}

int string_empty(string s)
{
  return string_size(s) == 0;
}

int string_null(string s)
{
  return s == string_null_object;
}

/* element access */

data string_data(string s)
{
  return data_construct(s, string_size(s));
}

data string_find_data(string s, data data)
{
  return string_find_at_data(s, 0, data);
}

data string_find_at_data(string s, size_t position, data data)
{
  void *p;

  p = data_empty(data) ? NULL : memmem(s + position, string_size(s) - position, data_base(data), data_size(data));
  return p ? data_construct(p, data_size(data)) : data_null();
}

/* modifiers */

string string_insert_data(string s, size_t offset, data data)
{
  s = string_allocate(s, string_size(s) + data_size(data));
  memmove(s + offset + data_size(data), s + offset, string_size(s) - offset);
  memcpy(s + offset, data_base(data), data_size(data));
  s = string_resize(s, string_size(s) + data_size(data));
  return s;
}

string string_prepend_data(string s, data data)
{
  return string_insert_data(s, 0, data);
}

string string_append_data(string s, data data)
{
  return string_insert_data(s, string_size(s), data);
}

string string_erase_data(string s, data data)
{
  size_t position;

  if (!data_empty(data))
  {
    position = data_offset(string_data(s), data);
    memmove(s + position, s + position + data_size(data), string_size(s) - position - data_size(data));
    s = string_resize(s, string_size(s) - data_size(data));
  }
  return s;
}

string string_replace_data(string s, data match, data replace)
{
  data data;
  size_t position;

  data = string_find_data(s, match);
  if (!data_empty(data))
  {
    position = data_offset(string_data(s), data);
    s = string_erase_data(s, data);
    s = string_insert_data(s, position, replace);
  }
  return s;
}

string string_replace_all_data(string s, data match, data replace)
{
  data data;
  size_t position;

  position = 0;
  while (position < string_size(s))
  {
    data = string_find_at_data(s, position, match);
    if (data_empty(data))
      break;
    position = data_offset(string_data(s), data);
    s = string_erase_data(s, data);
    s = string_insert_data(s, position, replace);
    position += data_size(replace);
  }
  return s;
}

/* operations */

int string_equal(string s1, string s2)
{
  size_t l1 = string_size(s1), l2 = string_size(s2);
  return l1 == l2 && memcmp(s1, s2, l1) == 0;
}

void string_write(string s, int fd)
{
  size_t offset;
  ssize_t n;

  offset = 0;
  while (offset < string_size(s))
  {
    n = write(fd, s + offset, string_size(s) - offset);
    assert(n > 0);
    offset -= n;
  }
}

int string_save(string s, const char *path)
{
  int fd;

  fd = open(path, O_WRONLY);
  if (fd == -1)
    return -1;

  string_write(s, fd);
  (void) close(fd);
  return 0;
}
