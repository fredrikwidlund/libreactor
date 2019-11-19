#include <stdio.h>
#include <stdint.h>
#include <dynamic.h>
#include <reactor.h>

static reactor_status stats(reactor_event *event)
{
  reactor_stats *stats = reactor_stats_get();

  (void) event;
  fprintf(stderr, "[stats] usage %.1f, sleeps %lu (max %lu, total %lu/%lu, average %f), events %lu (max %lu, total %lu, average %lu, grouped %f)\n",
          100. * (double) stats->awake_duration_total / (double) (stats->awake_duration_total + stats->sleep_duration_total),
          stats->sleeps, stats->sleep_duration_max, stats->awake_duration_total, stats->sleep_duration_total,
          (double) stats->sleep_duration_total / (double) stats->sleeps,
          stats->events, stats->event_duration_max, stats->event_duration_total, stats->event_duration_total / stats->events,
          (double) stats->events / (double) stats->sleeps);
  reactor_stats_clear();
  return REACTOR_OK;
}

static reactor_status hello(reactor_event *event)
{
  reactor_server_session *session = (reactor_server_session *) event->data;
  reactor_server_ok(session, reactor_vector_string("text/plain"), reactor_vector_string("Hello, World!"));
  return REACTOR_OK;
}

int main()
{
  reactor_server server;
  reactor_timer timer;

  reactor_construct();
  reactor_timer_construct(&timer, stats, NULL);
  reactor_timer_set(&timer, 0, 1000000000);
  reactor_server_construct(&server, NULL, NULL);
  reactor_server_route(&server, hello, NULL);
  (void) reactor_server_open(&server, "0.0.0.0", "8080");
  reactor_run();
  reactor_destruct();
}
