#ifndef _LT_TIME_
#define _LT_TIME_
#include <stdint.h>

uint64_t tick_diff_usecs(uint64_t t1, uint64_t t2);
uint64_t tick_diff_msecs(uint64_t t1, uint64_t t2);
uint64_t tick_diff_secs(uint64_t t1, uint64_t t2);
uint64_t rdtsc(void);

#endif
