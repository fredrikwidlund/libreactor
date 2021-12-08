#include <string.h>

#include "string.h"
#include "utility.h"

string string_segment(const char *base, size_t size)
{
  return (string) {.base = base, .size = size};
}

string string_constant(const char *base)
{
  return (string) {.base = base, .size = strlen(base)};
}

string string_integer(uint32_t integer, char *string_storage)
{
  size_t length = utility_u32_len(integer);
  utility_u32_sprint(integer, string_storage + length);
  return string_segment(string_storage, length);
}

const char *string_base(string string)
{
  return string.base;
}

size_t string_size(string string)
{
  return string.size;
}

void string_push(string string, char **base)
{
  memcpy(*base, string_base(string), string_size(string));
  *base += string_size(string);
}

void string_push_char(char c, char **base)
{
  **base = c;
  (*base)++;
}
