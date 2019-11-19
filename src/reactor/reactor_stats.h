#ifndef REACTOR_STATS_H_INCLUDED
#define REACTOR_STATS_H_INCLUDED

typedef struct reactor_stats reactor_stats;

struct reactor_stats
{
  int      active;
  uint64_t sleeps;
  uint64_t sleep_duration_max;
  uint64_t sleep_duration_total;
  uint64_t awake_duration_total;
  uint64_t events;
  uint64_t event_duration_max;
  uint64_t event_duration_total;
};

void           reactor_stats_sleep_start(void);
void           reactor_stats_sleep_end(int);
void           reactor_stats_event_start(void);
void           reactor_stats_event_end(void);
void           reactor_stats_clear(void);
reactor_stats *reactor_stats_get(void);

#endif /* REACTOR_STATS_H_INCLUDED */
