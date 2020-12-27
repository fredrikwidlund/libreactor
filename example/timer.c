#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/inotify.h>

#include <dynamic.h>
#include <reactor.h>

struct state
{
  timer timer;
  int count;
  uint64_t tsc;
};

static core_status callback(core_event *event)
{
  struct state *state = event->state;
  int64_t t;

  switch (event->type)
  {
  case TIMER_ALARM:
    if (state->tsc == 0)
      state->tsc = utility_tsc();
    else
    {
      t = utility_tsc();
      fprintf(stderr, "%.02fGhz\n", (double) (t - state->tsc) / 1000000000.);
      state->tsc = t;
      state->count--;
    }

    if (state->count)
      return CORE_OK;
    /* fall through */
  default:
    timer_destruct(&state->timer);
    return CORE_ABORT;
  }
}

int main()
{
  struct state state = {.count = 10};

  core_construct(NULL);
  timer_construct(&state.timer, callback, &state);
  timer_set(&state.timer, 0, 1000000000);
  core_loop(NULL);
  core_destruct(NULL);
}
