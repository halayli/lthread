/*
  lthread
  (C) 2011  Hasan Alayli <halayli@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  time_utils.c
*/
#include "time_utils.h"

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

