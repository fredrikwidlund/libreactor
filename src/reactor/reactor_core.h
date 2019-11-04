#ifndef REACTOR_REACTOR_CORE_H_INCLUDED
#define REACTOR_REACTOR_CORE_H_INCLUDED

#define REACTOR_CORE_MAX_EVENTS 32

void       reactor_core_construct(void);
void       reactor_core_destruct(void);
void       reactor_core_run(void);
void       reactor_core_add(reactor_user *, int, int);
void       reactor_core_modify(reactor_user *, int, int);
void       reactor_core_deregister(reactor_user *);
void       reactor_core_delete(reactor_user *, int);
reactor_id reactor_core_schedule(reactor_user_callback *, void *);
void       reactor_core_cancel(reactor_id);
uint64_t   reactor_core_now(void);

#endif /* REACTOR_REACTOR_CORE_H_INCLUDED */
