#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/epoll.h>

#include "reactor.h"

struct state
{
  reactor_handler input;
  reactor_handler output;
  char            buffer[4096];
  data            remaining;
};

int fill(struct state *state)
{
  ssize_t n;

  n = read(0, state->buffer, sizeof state->buffer);
  if (n == 0)
  {
    reactor_delete(&state->input, 0);
    reactor_delete(&state->output, 1);
    return -1;
  }
  if (n == -1 && errno == EAGAIN)
    return -1;
  assert(n > 0);
  state->remaining = data_construct(state->buffer, n);
  reactor_modify(&state->input, 0, 0);
  reactor_modify(&state->output, 1, EPOLLOUT | EPOLLET);
  return 0;
}

int flush(struct state *state)
{
  ssize_t n;

  n = write(1, data_base(state->remaining), data_size(state->remaining));
  if (n == -1 && errno == EAGAIN)
    return -1;
  assert(n > 0);
  state->remaining = data_select(state->remaining, n, data_size(state->remaining) - n);
  if (!data_size(state->remaining))
  {
    reactor_modify(&state->input, 0, EPOLLIN | EPOLLET);
    reactor_modify(&state->output, 1, 0);
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
  reactor_handler_construct(&state.input, input, &state);
  reactor_handler_construct(&state.output, output, &state);
  reactor_add(&state.input, 0, EPOLLIN);
  reactor_add(&state.output, 1, EPOLLOUT);
  reactor_loop();
  reactor_destruct();
}
