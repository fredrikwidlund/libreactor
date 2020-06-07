#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include <dynamic.h>
#include <reactor.h>

char r[] =
  "GET /path HTTP/1.0\r\n"
  "Host: test.com\r\n"
  "User-Agent: benchmark\r\n"
  "Accept: */*\r\n"
  "\r\n";

int main()
{
  char input[1024];
  char output[1024];
  http_request request;
  http_response response;
  uint64_t t1, t2;
  size_t i, n, iterations = 100000000, len;

  strcpy(input, r);
  len = strlen(input);

  t1 = utility_tsc();
  for (i = 0; i < iterations; i ++)
    {
      n = http_request_read(&request, (segment) {input, len});
      assert(n == len);
    }
  t2 = utility_tsc();
  (void) fprintf(stderr, "< %f\n", (double) (t2 - t1) / iterations);

  t1 = utility_tsc();
  for (i = 0; i < iterations; i ++)
    {
      http_response_ok(&response, segment_string("text/plain"), segment_string("Hello, World!"));
      n = http_response_size(&response);
      assert(n < sizeof output);
      http_response_write(&response, (segment) {output, n});
    }
  t2 = utility_tsc();
  (void) fprintf(stderr, "> %f\n", (double) (t2 - t1) / iterations);
}
