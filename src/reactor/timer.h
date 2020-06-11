#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

enum
{
  TIMER_ERROR,
  TIMER_ALARM
};

typedef struct timer timer;
struct timer
{
  core_handler user;
  int          fd;
  int          next;
};

void timer_construct(timer *, core_callback *, void *);
void timer_destruct(timer *);
void timer_set(timer *, uint64_t, uint64_t);
void timer_clear(timer *);

#endif /* TIMER_H_INCLUDED */
