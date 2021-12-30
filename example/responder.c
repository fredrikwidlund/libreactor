#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>

#include <reactor.h>

char date[30] = "Mon, 06 Dec 2021 19:08:57 GMT";

static void t1(stream *s, char *text)
{
  const char reply[] =
    "HTTP/1.1 200 OK\r\n"
    "Server: reactor\r\n"
    "Date: Mon, 06 Dec 2021 19:08:57 GMT\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "\r\n"
    "Hello, World!";
  char *base;

  (void) text;
  base = stream_allocate(s, strlen(reply));
  memcpy(base, reply, strlen(reply));
  memcpy(base + 40, date, 29);
}

static void t2(stream *s, char *text)
{
  const char header[] =
    "HTTP/1.1 200 OK\r\n"
    "Server: reactor\r\n"
    "Date: Mon, 06 Dec 2021 19:08:57 GMT\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: ";
  size_t header_size = 113;
  size_t body_size = strlen(text);
  size_t length_size = utility_u32_len(body_size);
  char *reply;

  reply = stream_allocate(s, header_size + length_size + 4 + body_size);
  memcpy(reply, header, header_size);
  memcpy(reply + 40, date, 29);
  utility_u32_sprint(body_size, reply + header_size + length_size);
  memcpy(reply + header_size + length_size, "\r\n\r\n", 4);
  memcpy(reply + header_size + length_size + 4, text, body_size);
}

static void pushs(char **base, const char *data)
{
  memcpy(*base, data, strlen(data));
  *base += strlen(data);
}

static void push(char **base, const char *data, size_t size)
{
  memcpy(*base, data, size);
  *base += size;
}

static void t3(stream *s, char *text)
{
  char header[] =
    "HTTP/1.1 200 OK\r\n"
    "Server: reactor\r\n"
    "Date: Mon, 06 Dec 2021 19:08:57 GMT\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: ";
  size_t header_size = 113;
  size_t body_size = strlen(text);
  size_t length_size = utility_u32_len(body_size);
  char *reply, *p;

  p = reply = stream_allocate(s, header_size + length_size + 4 + body_size);
  push(&p, header, header_size);
  memcpy(reply + 40, date, 29);
  utility_u32_sprint(body_size, p + length_size);
  push(&p, "\r\n\r\n", 4);
  push(&p, text, body_size);
}

static void t4(stream *s, char *text)
{
  char *p = stream_allocate(s, 132);
  size_t size = strlen(text);

  pushs(&p,
        "HTTP/1.1 200 OK\r\n"
        "Server: reactor\r\n"
        "Date: "
    );
  pushs(&p, date);
  pushs(&p,
        "\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: ");
  p += utility_u32_len(size);
  utility_u32_sprint(size, p);
  pushs(&p, "\r\n\r\n");
  push(&p, text, size);
}

static void pushc(char **p, char c)
{
  **p = c;
  (*p)++;
}

static void pushf(char **p, char *name, char *value)
{
  size_t name_len = strlen(name);
  size_t value_len = strlen(value);

  push(p, name, name_len);
  pushc(p, ':');
  pushc(p, ' ');
  push(p, value, value_len);
  pushc(p, '\r');
  pushc(p, '\n');
}

static void pushf2(char **p, char *name, size_t name_len, char *value, size_t value_len)
{
  push(p, name, name_len);
  pushc(p, ':');
  pushc(p, ' ');
  push(p, value, value_len);
  pushc(p, '\r');
  pushc(p, '\n');
}

static void t5(stream *s, char *text)
{
  char *p = stream_allocate(s, 132);
  size_t size = strlen(text);

  pushs(&p, "HTTP/1.1 200 OK\r\n");
  pushf(&p, "Server", "reactor");
  pushf(&p, "Date", date);
  pushf(&p, "Content-Type", "text/plain");
  pushs(&p, "Content-Length: ");
  p += utility_u32_len(size);
  utility_u32_sprint(size, p);
  pushs(&p, "\r\n\r\n");
  pushs(&p, text);
}

static void t6(stream *s, char *text)
{
  char *p = stream_allocate(s, 132);
  size_t size = strlen(text);

  push(&p, "HTTP/1.1 200 OK\r\n", 17);
  pushf2(&p, "Server", 6, "reactor", 7);
  pushf2(&p, "Date", 4, date, 29);
  pushf2(&p, "Content-Type", 12, "text/plain", 10);
  push(&p, "Content-Length: ", 17);
  p += utility_u32_len(size);
  utility_u32_sprint(size, p);
  push(&p, "\r\n\r\n", 4);
  push(&p, text, size);
}

static void push_standard_200_header(char **p)
{
  push(p, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"));
  pushf2(p, "Server", 6, "reactor", 7);
  pushf2(p, "Date", 4, date, 29);
}

static void push_body(char **p, char *type, size_t type_size, char *data, size_t data_size)
{
  pushf2(p, "Content-Type", 12, type, type_size);
  push(p, "Content-Length: ", strlen("Content-Length: "));
  *p += utility_u32_len(data_size);
  utility_u32_sprint(data_size, *p);
  push(p, "\r\n\r\n", 4);
  push(p, data, data_size);  
}

static void t8(stream *s, char *text, size_t size)
{
  char *p = stream_allocate(s, 132);

  push_standard_200_header(&p);
  push_body(&p, "text/plain", strlen("test/plain"), text, size);
}

struct pair
{
  char *name;
  char *value;
};

static void pusharray(char **p, struct pair *pair, size_t n)
{
  size_t i;

  for (i = 0; i < n; i ++)
    pushf(p, pair[i].name, pair[i].value);
}

static void t7(stream *s, char *text)
{
  char *p = stream_allocate(s, 132);
  size_t size = strlen(text);

  pushs(&p, "HTTP/1.1 200 OK\r\n");
  pusharray(&p, (struct pair[])
            {{"Server", "reactor"},
              {"Date", date},
              {"Content-Type", "text/plain"}}, 3);
  pushs(&p, "Content-Length: ");
  p += utility_u32_len(size);
  utility_u32_sprint(size, p);
  pushs(&p, "\r\n\r\n");
  pushs(&p, text);
}

void measure(int n, char *name, void (*f)(stream *, char *))
{
  stream s;
  int i;
  uint64_t t1, t2;

  stream_construct(&s, NULL, NULL);
  t1 = utility_tsc();
  for (i = 0; i < n; i++)
  {
    f(&s, "Hello, World!");
    buffer_clear(&s.output);
  }
  t2 = utility_tsc();
  stream_destruct(&s);
  printf("[%s] %lu\n", name, (t2 - t1) / n);
}

void measure2(int n, char *name, void (*f)(stream *, char *, size_t))
{
  stream s;
  int i;
  uint64_t t1, t2;

  stream_construct(&s, NULL, NULL);
  t1 = utility_tsc();
  for (i = 0; i < n; i++)
  {
    f(&s, "Hello, World!", strlen("Hello, World!"));
    buffer_clear(&s.output);
  }
  t2 = utility_tsc();
  stream_destruct(&s);
  printf("[%s] %lu\n", name, (t2 - t1) / n);
}

static void segment_push_200(pointer *p)
{
  pointer_push(p, data_string("HTTP/1.1 200 OK\r\n"));
  http_field_push(p, http_field_construct(data_string("Server"), data_string("reactor")));
  http_field_push(p, http_field_construct(data_string("Date"), data_construct(date, 29)));
}


static void segment_push_body(pointer *p, data type, data body)
{
  http_field_push(p, http_field_construct(data_string("Content-Type"), type));
  pointer_push(p, data_string("Content-Length: "));
  pointer_move(p, utility_u32_len(data_size(body)));
  utility_u32_sprint(data_size(body), *p);
  pointer_push(p, data_string("\r\n\r\n"));
  pointer_push(p, body);
}

static void segment_measure(stream *s, data text)
{
  pointer p = stream_allocate(s, 132);

  segment_push_200(&p);
  segment_push_body(&p, data_string("text/plain"), text);
}

void measure3(int n, char *name, void (*f)(stream *, data))
{
  stream s;
  int i;
  uint64_t t1, t2;

  stream_construct(&s, NULL, NULL);
  buffer_resize(&s.output, 4096);
  t1 = utility_tsc();
  for (i = 0; i < n; i++)
  {
    f(&s, data_string("Hello, World!"));
    buffer_clear(&s.output);
  }
  t2 = utility_tsc();
  stream_destruct(&s);
  printf("[%s] %lu\n", name, (t2 - t1) / n);
}

int main()
{
  measure(10000000, "memcpy", t1);
  measure(10000000, "tailored", t2);
  measure(10000000, "push", t3);
  measure(10000000, "push2", t4);
  measure(10000000, "push3", t5);
  measure(10000000, "push4", t6);
  measure2(10000000, "push5", t8);
  measure(10000000, "push array", t7);
  measure3(10000000, "segment", segment_measure);
}
