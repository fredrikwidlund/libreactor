#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/epoll.h>

#include "reactor.h"

struct state
{
  descriptor input;
  descriptor output;
  char       buffer[4096];
  data       remaining;
};

int fill(struct state *state)
{
  ssize_t n;
  
  n = read(descriptor_fd(&state->input), state->buffer, sizeof state->buffer);
  if (n == 0)
  {
    descriptor_close(&state->input);
    descriptor_close(&state->output);
    return -1;
  }
  if (n == -1 && errno == EAGAIN)
    return -1;
  assert(n > 0);
  state->remaining = data_construct(state->buffer, n);
  descriptor_mask(&state->input, 0);
  descriptor_mask(&state->output, DESCRIPTOR_WRITE);
  return 0;
}

int flush(struct state *state)
{
  ssize_t n;

  n = write(descriptor_fd(&state->output), data_base(state->remaining), data_size(state->remaining));
  if (n == -1 && errno == EAGAIN)
    return -1;
  assert(n > 0);
  state->remaining = data_select(state->remaining, n, data_size(state->remaining) - n);
  if (!data_size(state->remaining))
  {
    descriptor_mask(&state->input, DESCRIPTOR_READ);
    descriptor_mask(&state->output, 0);
  }
  return 0;
}

void input(reactor_event *event)
{
  struct state *state = event->state;
  int e;

  while (!data_size(state->remaining))
  {
    e = fill(state);
    if (e == -1)
      break;
    e = flush(state);
    if (e == -1)
      break;
  }
}

void output(reactor_event *event)
{
  struct state *state = event->state;
  int e;
  
  while (data_size(state->remaining))
  {
    e = flush(state);
    if (e == -1)
      break;
  }
}

int main()
{
  struct state state = {0};

  fcntl(0, F_SETFL, O_NONBLOCK);
  fcntl(1, F_SETFL, O_NONBLOCK);
  
  reactor_construct();
  descriptor_construct(&state.input, input, &state);
  descriptor_construct(&state.output, output, &state);
  descriptor_open(&state.input, 0, DESCRIPTOR_READ);
  descriptor_open(&state.output, 1, DESCRIPTOR_READ);
  reactor_loop();
  reactor_destruct();
}
