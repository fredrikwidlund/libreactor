#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <dynamic.h>

#include "reactor_core.h"

void status(void *state, reactor_rest_request *request)
{
  reactor_rest_respond(request, 200,
                       1, (reactor_http_field[]) {{"Content-Type", "application/json"}},
                       strlen((char *) state), state);
}

int main()
{
  reactor_rest rest;

  reactor_core_open();
  reactor_rest_init(&rest, NULL, &rest);
  reactor_rest_server(&rest, "localhost", "80", 0);
  reactor_rest_add_match(&rest, "GET", "/status", status, "{\"status\":\"ok\"}");
  assert(reactor_core_run() == 0);
  reactor_core_close();
}
