#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/epoll.h>

#include "reactor.h"

struct state
{
  stream input;
  stream output;
};

void input(reactor_event *event)
{
  struct state *state = event->state;
  data data;

  switch (event->type)
  {
  case STREAM_READ:
    data = stream_read(&state->input);
    stream_write(&state->output, data);
    stream_flush(&state->output);
    stream_consume(&state->input, data_size(data));
    break;
  case STREAM_CLOSE:
    stream_close(&state->input);
    stream_notify(&state->output);
    break;
  }
}

void output(reactor_event *event)
{
  struct state *state = event->state;

  stream_close(&state->output);
}

int main()
{
  struct state state = {0};

  fcntl(0, F_SETFL, O_NONBLOCK);
  fcntl(1, F_SETFL, O_NONBLOCK);
  
  reactor_construct();
  stream_construct(&state.input, input, &state);
  stream_construct(&state.output, output, &state);
  stream_open(&state.input, 0, STREAM_READ, NULL);
  stream_open(&state.output, 1, STREAM_WRITE, NULL);
  reactor_loop();
  stream_destruct(&state.input);
  stream_destruct(&state.output);
  reactor_destruct();
}
