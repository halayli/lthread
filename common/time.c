#include "time.h"

uint64_t
tick_diff_usecs(uint64_t t1, uint64_t t2)
{
       uint64_t t3 =  0;
	t3 = ((long double)(t2 - t1)/2793008320u) * 1000000u;
       return t3;
}

uint64_t
tick_diff_msecs(uint64_t t1, uint64_t t2)
{
       uint64_t t3 =  0;
	t3 = ((long double)(t2 - t1)/2793008320u) * 1000000u;
       return t3/1000;
}

uint64_t
tick_diff_secs(uint64_t t1, uint64_t t2)
{
       uint64_t t3 = ((long double)(t2 - t1)/2793008320u) * 1000000u;
       return t3/1000000;
}

uint64_t
rdtsc(void)
{
  uint32_t a = 0, d = 0;

  asm volatile ("rdtsc" : "=a"(a), "=d"(d));
  return (((uint64_t) d << 32) | a);
}

