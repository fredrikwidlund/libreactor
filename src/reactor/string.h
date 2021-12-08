#ifndef REACTOR_STRING_H_INCLUDED
#define REACTOR_STRING_H_INCLUDED

#include <stdint.h>

typedef struct string string;

struct string
{
  const char *base;
  size_t      size;
};

/* constructor/destructor */

string      string_segment(const char *, size_t);
string      string_constant(const char *);
string      string_integer(uint32_t, char *);
void        string_push(string, char **);
void        string_push_char(char, char **);

/* capacity */

size_t      string_size(string);

/* modifiers */

/* element access */

const char *string_base(string);

#endif /* REACTOR_STRING_H_INCLUDED */
