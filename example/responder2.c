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

typedef struct string string;

struct string
{
  const char *base;
  size_t      size;
};

struct strings
{
  buffer      buffer;
};

const char *string_base(string string)
{
  return string.base;
}

size_t string_size(string string)
{
  return string.size;
}


static string string_memory(const char *base, size_t size)
{
  return (string) {.base = base, .size = size};
}

static string string_constant(const char *text)
{
  return string_memory(text, strlen(text));
}

static void string_write(char **base, string string)
{
  memcpy(*base, string_base(string), string_size(string));
  *base += string_size(string);
}

static void http_write_field(char **base, string name, string value)
{
  string_write(base, name);
  pushc(base, ':');
  pushc(base, ' ');
  string_write(base, value);
  pushc(base, '\r');
  pushc(base, '\n');
}

static void http_write_header(char **base, int code)
{
  string_write(base, string_constant("HTTP/1.1 200 OK\r\n"));
  http_write_field(base, string_constant("Server"), string_constant("reactor"));
  http_write_field(base, string_constant("Date"), string_memory(date, 29));
}

static void segment_push_body(char **p, string type, string data)
{
  http_write_field(p, string_constant("Content-Type"), type);
  string_write(p, string_constant("Content-Length: "));
  *p += utility_u32_len(string_size(data));
  utility_u32_sprint(string_size(data), *p);
  string_write(p, string_constant("\r\n\r\n"));
  push(p, string_base(data), string_size(data));
}

static void segment_measure(stream *s, string text)
{
  char *p = stream_allocate(s, 132);

  http_write_header(&p, 200);
  segment_push_body(&p, string_constant("text/plain"), text);
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
