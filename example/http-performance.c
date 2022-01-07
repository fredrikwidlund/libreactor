#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>

#include <reactor.h>

static void update_date(char *date)
{
  time_t t;
  struct tm tm;
  static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  t = time(NULL);
  (void) gmtime_r(&t, &tm);
  (void) strftime(date, 30, "---, %d --- %Y %H:%M:%S GMT", &tm);
  memcpy(date, days[tm.tm_wday], 3);
  memcpy(date + 8, months[tm.tm_mon], 3);
}

int main()
{
  char date[29];
  stream s;
  uint64_t t0, t1;
  int i, rounds = 10000000, status_code;
  ssize_t n;
  data method, target, status;
  http_field fields[16];
  size_t fields_count;

  update_date(date);
  stream_construct(&s, NULL, NULL);

  /* write request */
  
  t0 = utility_tsc();
  for (i = 0; i < rounds; i ++)
  {
    buffer_clear(&s.output);
    http_write_request(&s, data_string("POST"), data_string("/"), data_string("localhost"), data_string("text/plain"), data_string("Hello, World!"));
  }
  t1 = utility_tsc();
  printf("[write request %lu] %lu\n", buffer_size(&s.output), (t1 - t0) / rounds);
  printf("%.*s\n", (int) s.output.size, (char *) s.output.data); 

  /* read request */
  
  fields_count = 16;
  buffer_insert(&s.input, 0, s.output.data, s.output.size);
  buffer_clear(&s.output);
  t0 = utility_tsc();
  for (i = 0; i < rounds; i ++)
  {
    buffer_clear(&s.output);
    n = http_read_request(&s, &method, &target, fields, &fields_count);
  }
  t1 = utility_tsc();
  printf("[read request %ld] %lu\n", n, (t1 - t0) / rounds);
  printf("Method: %.*s\n", (int) method.size, (char *) method.base);
  printf("Target: %.*s\n", (int) target.size, (char *) target.base);
  for (i = 0; i < (int) fields_count; i ++)
    printf("%.*s: %.*s\n",
           (int) fields[i].name.size, (char *) fields[i].name.base,
           (int) fields[i].value.size, (char *) fields[i].value.base);


  t0 = utility_tsc();
  for (i = 0; i < rounds; i ++)
  {
    buffer_clear(&s.output);
    http_write_response(&s, data_string("200 OK"), data_construct(date, sizeof date), data_string("text/html"), data_string("Hello, World!"));
  }
  t1 = utility_tsc();
  printf("[write response %lu] %lu\n", buffer_size(&s.output), (t1 - t0) / rounds);
  printf("%.*s\n", (int) s.output.size, (char *) s.output.data); 
 
  fields_count = 16;
  buffer_insert(&s.input, 0, s.output.data, s.output.size);
  buffer_clear(&s.output);
  t0 = utility_tsc();
  for (i = 0; i < rounds; i ++)
  {
    buffer_clear(&s.output);
    n = http_read_response(&s, &status_code, &status, fields, &fields_count);
  }
  t1 = utility_tsc();
  printf("[read response %ld] %lu\n", n, (t1 - t0) / rounds);
  printf("Status: %d %.*s\n", status_code, (int) status.size, (char *) status.base);
  for (i = 0; i < (int) fields_count; i ++)
    printf("%.*s: %.*s\n",
           (int) fields[i].name.size, (char *) fields[i].name.base,
           (int) fields[i].value.size, (char *) fields[i].value.base);
  
  stream_destruct(&s);
}
