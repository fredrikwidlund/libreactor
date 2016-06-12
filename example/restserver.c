#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <dynamic.h>

#include "reactor_core.h"

void hello_world(void *state, reactor_rest_request *request)
{
  (void) state;
  reactor_rest_text(request, "hello world");
}

void quit(void *state, reactor_rest_request *request)
{
  (void) request;
  reactor_rest_close(state);
}

int main()
{
  reactor_rest rest;

  reactor_core_open();
  reactor_rest_init(&rest, NULL, NULL);
  reactor_rest_open(&rest, "localhost", "80", 0);
  reactor_rest_add_match(&rest, "GET", "/", hello_world, NULL);
  reactor_rest_add_match(&rest, "GET", "/quit", quit, &rest);
  assert(reactor_core_run() == 0);
  reactor_core_close();
}
