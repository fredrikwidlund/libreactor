#include <stdint.h>

#include "reactor_stats.h"

static __thread reactor_stats stats = {0};
static uint64_t t0 = 0, t1 = 0;

static uint64_t read_tsc(void)
{
  uint32_t lo, hi;

  __asm__ volatile ("RDTSC" : "=a" (lo), "=d" (hi));

  return (((uint64_t) hi) << 32) | lo;
}

void reactor_stats_sleep_start(void)
{
  uint64_t t;

  if (!stats.active)
    stats = (reactor_stats) {.active = 1};

  t = read_tsc();
  if (t0)
    stats.awake_duration_total += t - t0;
  t0 = t;

  stats.sleeps ++;
}

void reactor_stats_sleep_end(int events)
{
  uint64_t t, d;

  t = read_tsc();
  d = t - t0;
  t0 = t;

  stats.events += events;
  stats.sleep_duration_total += d;
  if (d > stats.sleep_duration_max)
    stats.sleep_duration_max = d;
}

void reactor_stats_event_start(void)
{
  t1 = read_tsc();
}

void reactor_stats_event_end(void)
{
  uint64_t d;

  d = read_tsc() - t1;

  stats.event_duration_total += d;
  if (d > stats.event_duration_max)
    stats.event_duration_max = d;
}

void reactor_stats_clear(void)
{
  stats.active = 0;
}

reactor_stats *reactor_stats_get(void)
{
  return &stats;
}
