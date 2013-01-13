#ifndef LTHREAD_TIME
#define LTHREAD_TIME
#include <stdint.h>

uint64_t _lthread_tick_diff_usecs(uint64_t t1, uint64_t t2);
uint64_t _lthread_tick_diff_msecs(uint64_t t1, uint64_t t2);
uint64_t _lthread_tick_diff_secs(uint64_t t1, uint64_t t2);
#ifdef __i386__
uint32_t _lthread_rdtsc(void);
# else
uint64_t _lthread_rdtsc(void);
# endif
int    _lthread_rdtsc_init(void);

#endif
