#ifndef REACTOR_REACTOR_UTILITY_H_INCLUDED
#define REACTOR_REACTOR_UTILITY_H_INCLUDED

size_t         reactor_utility_u32len(uint32_t);
void           reactor_utility_u32sprint(uint32_t, char *);
void           reactor_utility_u32toa(uint32_t, char *);
reactor_vector reactor_utility_u32tov(uint32_t);

#endif /* REACTOR_REACTOR_UTILITY_H_INCLUDED */
