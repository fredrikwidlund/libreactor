#include "reactor.h"

int main()
{
  string s1, s2, s3;

  s1 = string_read(0);
  string_write(s1, 1);
  string_free(s1);

  s2 = string_append_data(string_new(), data_string("xxx"));
  s1 = string_append_data(string_new(), data_string("test1"));
  s1 = string_prepend_data(s1, data_string(">>>"));

  s1 = string_append_data(s1, data_string("test2"));
  s1 = string_append_data(s1, string_data(s2));

  s3 = string_copy(s1);
  s1 = string_append_data(s1, data_string("-"));
  s1 = string_append_data(s1, string_data(s3));

  data d = string_find_data(s1, data_string("test"));
  printf("found %d\n", !data_empty(d));

  int i;
  for (i = 0; i < 10; i++)
    s1 = string_replace_all_data(s1, data_string("test"), data_string("<test>"));

  printf("%s\n", s1);

  string_free(s1);
  string_free(s2);
  string_free(s3);
}
