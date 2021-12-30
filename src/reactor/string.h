#ifndef REACTOR_STRING_H_INCLUDED
#define REACTOR_STRING_H_INCLUDED

#include <stdint.h>

typedef char *string;

/* allocators */

string string_new(void);
string string_copy(string);
void   string_free(string);
string string_read(int);
string string_load(const char *);

/* capacity */

string string_allocate(string, size_t);
string string_resize(string, size_t);
size_t string_size(string);
int    string_empty(string);
int    string_null(string);

/* element access */

data   string_data(string);
data   string_find_data(string, data);
data   string_find_at_data(string, size_t, data);

/* modifiers */

string string_insert_data(string, size_t, data);
string string_erase_data(string, data);
string string_prepend_data(string, data);
string string_append_data(string, data);
string string_replace_data(string, data, data);
string string_replace_all_data(string, data, data);

/* operations */

int    string_equal(string, string);
void   string_write(string, int);
int    string_save(string , const char *);

#endif /* REACTOR_STRING_H_INCLUDED */
