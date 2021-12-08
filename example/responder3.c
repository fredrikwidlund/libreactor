#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>

#include <reactor.h>

char date[30] = "Mon, 06 Dec 2021 19:08:57 GMT";

static void push(char **base, const char *data, size_t size)
{
  memcpy(*base, data, size);
  *base += size;
}

static void pushc(char **p, char c)
{
  **p = c;
  (*p)++;
}

typedef struct http_field2 http_field2;
struct http_field2
{
  string name;
  string value;
};

static void http_field_write(char **base, http_field2 field)
{
  string_push(field.name, base);
  pushc(base, ':');
  pushc(base, ' ');
  string_push(field.value, base);
  pushc(base, '\r');
  pushc(base, '\n');
}

static http_field2 http_field_construct(string name, string value)
{
  return (http_field2) {.name = name, .value = value};
}

static size_t http_field_write_size(http_field2 field)
{
  return field.name.size + 2 + field.value.size + 2;
}

static http_field2 http_field_chars(char *name, char *value)
{
  return (http_field2) {.name = string_constant(name), .value = string_constant(value)};
}

static void http_respond_ok(stream *stream, string type, string body)
{
  http_field2 server_field = http_field_chars("Server", "reactor");
  http_field2 date_field = http_field_construct(string_constant("Date"), string_segment(date, 29));
  size_t length_size = utility_u32_len(string_size(body));
  size_t size =
    http_field_write_size(server_field) +
    http_field_write_size(date_field) +
    37 + (16 + string_size(type)) + (20 + length_size) + 2 + string_size(body);
  char *base = stream_allocate(stream, size);
  char length[16];

  utility_u32_sprint(string_size(body), length + length_size);
  string_push(string_constant("HTTP/1.1 200 OK\r\n"), &base);
  
  http_field_write(&base, server_field);
  http_field_write(&base, date_field);
  http_field_write(&base, http_field_construct(string_constant("Content-Type"), type));
  http_field_write(&base, http_field_construct(string_constant("Content-Length"), string_segment(length, length_size)));
  string_push(string_constant("\r\n"), &base);
  push(&base, string_base(body), string_size(body));
}

static void segment_measure(stream *stream, string text)
{
  http_respond_ok(stream, string_constant("text/plain"), text);
}

void measure3(int n, char *name, void (*f)(stream *, string))
{
  stream s;
  int i;
  uint64_t t1, t2;
  
  stream_construct(&s, NULL, NULL);
  buffer_resize(&s.output, 4096);
  t1 = utility_tsc();
  for (i = 0; i < n; i++)
  {
    f(&s, string_constant("Hello, World!"));
    buffer_clear(&s.output);
  }
  t2 = utility_tsc();
  stream_destruct(&s);
  printf("[%s] %lu\n", name, (t2 - t1) / n);
}




int main()
{
  /*
  measure(10000000, "memcpy", t1);
  measure(10000000, "tailored", t2);
  measure(10000000, "push", t3);
  measure(10000000, "push2", t4);
  measure(10000000, "push3", t5);
  measure(10000000, "push4", t6);
  */
  //measure2(10000000, "push5", t8);
  //measure(10000000, "push array", t7);
  measure3(10000000, "segment", segment_measure);
}
