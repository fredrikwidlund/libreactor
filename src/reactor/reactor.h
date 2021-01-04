#ifndef REACTOR_REACTOR_H_INCLUDED
#define REACTOR_REACTOR_H_INCLUDED

void reactor_construct(void);
void reactor_destruct(void);
void reactor_loop(void);
int  reactor_clone(int);
int  reactor_affinity(int);

#endif /* REACTOR_REACTOR_H_INCLUDED */
