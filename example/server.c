#include <stdio.h>
#include <signal.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

typedef struct hello hello;
struct hello
{
  server server;
  timer  timer;
  size_t requests;
};

static core_status request(core_event *event)
{
  hello *hello = event->state;

  if (event->type != SERVER_REQUEST)
    err(1, "server");

  server_ok((server_context *) event->data, segment_string("text/plain"), segment_string("Hello, World!"));
  hello->requests++;
  return CORE_OK;
}

static core_status timeout(core_event *event)
{
  hello *hello = event->state;
  core_counters *counters;

  if (event->type != TIMER_ALARM)
    err(1, "timer");

  counters = core_get_counters(NULL);
  (void) fprintf(stderr, "[hello] requests %lu, usage %.02f%%, frequency %.02fGHz\n",
                 hello->requests, 100. * (double) counters->awake / (double) (counters->awake + counters->sleep),
                 (double) (counters->awake + counters->sleep) / 1000000000.0);
  core_clear_counters(NULL);
  hello->requests = 0;
  return CORE_OK;
}

int main()
{
  hello hello = {0};

  signal(SIGPIPE, SIG_IGN);
  core_construct(NULL);
  server_construct(&hello.server, request, &hello);
  server_open(&hello.server, 0, 80);
  timer_construct(&hello.timer, timeout, &hello);
  timer_set(&hello.timer, 1000000000, 1000000000);
  core_loop(NULL);
  core_destruct(NULL);
}
