#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>

#include <reactor.h>

typedef struct request request;
struct request
{
  int      fd[2];
  stream   stream;
  server   server;
  uint64_t start;
  uint64_t stop;
  size_t   count;
};

static void request_write(request *r)
{
  static char request[] =
    "GET / HTTP/1.1\r\n"
    "Host: server\r\n"
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) Gecko/20130501 Firefox/30.0 AppleWebKit/600.00 Chrome/30.0.0000.0 Trident/10.0 Safari/600.00\r\n"
    "Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    "Accept-Language: en-US,en;q=0.5\r\n"
    "Connection: keep-alive\r\n"
    "\r\n";

  stream_write(&r->stream, data_string(request));
  stream_flush(&r->stream);
}

static void response_read(request *r)
{
  data data;

  r->count++;
  data = stream_read(&r->stream);
  if (!data_prefix(data_string("HTTP/1.1 200 OK\r\n"), data))
    assert(0);
  stream_consume(&r->stream, data_size(data));
  if (reactor_now() > r->start + 1000000000)
  {
    r->stop = reactor_now();
    stream_close(&r->stream);
    server_close(&r->server);
  }
  else
    request_write(r);
}

static void request_stream_callback(reactor_event *event)
{
  request *r = event->state;

  switch (event->type)
  {
  case STREAM_WRITE:
    r->start = reactor_now();
    request_write(r);
    break;
  case STREAM_READ:
    response_read(r);
    break;
  }
}

static void request_server_callback(reactor_event *event)
{
  server_transaction_text((server_transaction *) event->data, data_string("Hello world"));
}

int main()
{
  request r = {0};
  int e;

  e = socketpair(AF_UNIX, SOCK_STREAM, 0, r.fd);
  assert(e == 0);

  reactor_construct();
  stream_construct(&r.stream, request_stream_callback, &r);
  stream_open(&r.stream, r.fd[0], NULL, 1);
  server_construct(&r.server, request_server_callback, &r);
  server_accept(&r.server, r.fd[1], NULL);
  reactor_loop();

  stream_destruct(&r.stream);
  server_destruct(&r.server);
  reactor_destruct();

  printf("%.0f rps\n", (double) r.count / ((double) (r.stop - r.start) / 1000000000.0));
}
