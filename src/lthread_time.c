/*
 * Lthread
 * Copyright (C) 2012, Hasan Alayli <halayli@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * lthread_time.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lthread_time.h"

static uint64_t cpu_freq = 1801800000L;

#if defined(__FreeBSD__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
int
_lthread_rdtsc_init(void)
{
    char buf[8];
    size_t buf_size = 0;
    int ret = 0;
    #if defined(__APPLE__)
    ret = sysctlbyname("machdep.tsc.frequency", buf, &buf_size, NULL, 0);
    #else
    ret = sysctlbyname("machdep.tsc_freq", buf, &buf_size, NULL, 0);
    # endif
    if (ret == 0) {
        if (buf_size == 4)
            cpu_freq = *(uint32_t*)buf;
        if (buf_size == 8)
            cpu_freq = *(uint64_t*)buf;
    }

    return (ret);
}
#else
int
_lthread_rdtsc_init(void)
{
    FILE *fp = NULL;
    #define MAXLEN 1024
    char line[MAXLEN];
    char *tmp = NULL;
    int found = -1;

    if ((fp = fopen("/proc/cpuinfo", "r")) == NULL)
        return (-1);

    while (fgets(line, MAXLEN, fp) != NULL) {
        if ((tmp = strstr(line, "cpu MHz")) != NULL) {
            if ((tmp = strchr(tmp, ':')) != NULL) {
                found = 0;
                tmp++;
                cpu_freq = strtod(tmp, NULL) * 1000000u;
                break;
            }
        }
    }
    fclose(fp);

    return found;
}
#endif

uint64_t
_lthread_tick_diff_usecs(uint64_t t1, uint64_t t2)
{
    uint64_t t3 =  0;
    t3 = ((long double)(t2 - t1)/cpu_freq) * 1000000u;
    return (t3);
}

uint64_t
_lthread_tick_diff_msecs(uint64_t t1, uint64_t t2)
{
       uint64_t t3 =  0;
	t3 = ((long double)(t2 - t1)/cpu_freq) * 1000000u;
       return (t3/1000);
}

uint64_t
_lthread_tick_diff_secs(uint64_t t1, uint64_t t2)
{
    uint64_t t3 = ((long double)(t2 - t1)/cpu_freq) * 1000000u;
    return (t3/1000000);
}

#ifdef __i386__
inline uint32_t
_lthread_rdtsc(void)
{
    uint32_t a;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (a));
    return (a);
}
#else
inline uint64_t
_lthread_rdtsc(void)
{
  uint32_t a = 0, d = 0;

  asm volatile ("rdtsc" : "=a"(a), "=d"(d));
  return (((uint64_t) d << 32) | a);
}
#endif
