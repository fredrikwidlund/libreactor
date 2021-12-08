#include <stdio.h>
#include <stdint.h>
#include <reactor.h>

struct state
{
  timer    timer;
  uint64_t expirations;
  uint64_t t0;
  uint64_t t1;
  uint64_t calls;
};

static void callback(reactor_event *event)
{
  struct state *state = event->state;

  state->calls++;
  if (event->data >= state->expirations)
  {
    state->t1 = reactor_now();
    timer_clear(&state->timer);
  }
  state->expirations -= event->data;
}

int main()
{
  struct state state = {.expirations = 1000000};
  
  reactor_construct();
  timer_construct(&state.timer, callback, &state);
  (void) fprintf(stderr, "timer started expiring every 1us\n");
  state.t0 = reactor_now();
  timer_set(&state.timer, 0, 1000);

  reactor_loop();

  (void) fprintf(stderr, "timer stopped after receiving 1000000 expirations, %luns later, "
                 "receiving on average %lu expirations/timeout\n", state.t1 - state.t0, 1000000 / state.calls);
  timer_destruct(&state.timer);
  reactor_destruct();
}
