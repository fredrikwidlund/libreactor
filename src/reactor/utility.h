#ifndef UTILITY_H_INCLUDED
#define UTILITY_H_INCLUDED

size_t   utility_u32_len(uint32_t);
void     utility_u32_sprint(uint32_t, char *);
void     utility_u32_toa(uint32_t, char *);

uint64_t utility_tsc(void);

#endif /* UTILITY_H_INCLUDED */
