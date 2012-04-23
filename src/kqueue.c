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
 * kqueue.c
 */

#include "lthread_int.h"
#include <stdio.h>
#include <assert.h>

inline void
register_rd_interest(int fd)
{
    sched_t *sched = lthread_get_sched();
    EV_SET(&sched->changelist[sched->nevents++], fd, EVFILT_READ,
        EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, sched->current_lthread);
    if (sched->current_lthread)
        sched->current_lthread->state |= bit(LT_WAIT_READ);
}

inline void
register_wr_interest(int fd)
{
    sched_t *sched = lthread_get_sched();
    EV_SET(&sched->changelist[sched->nevents++], fd, EVFILT_WRITE,
        EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, sched->current_lthread);
    sched->current_lthread->state |= bit(LT_WAIT_WRITE);
}

inline void
clear_interest(int fd)
{
    struct kevent change;
    struct kevent event;
    struct timespec tm = {0, 0};
    int ret = 0;
    sched_t *sched = lthread_get_sched();

    //printf("sched is %p and fd is %d\n", sched, fd);
    /*EV_SET(&sched->changelist[sched->nevents++],
        fd, 0, EV_DELETE, 0, 0, NULL);*/
    EV_SET(&change, fd, EVFILT_READ|EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    ret = kevent(sched->poller, &change, 1, &event, 0, &tm);
    //assert(ret == 0);

}

int
create_poller(void)
{
    return kqueue();
}

inline int
poll_events(struct timespec t)
{
    sched_t *sched = lthread_get_sched();
    return kevent(sched->poller, sched->changelist, sched->nevents,
        sched->eventlist, LT_MAX_EVENTS, &t);
}

inline int
get_fd(struct kevent *ev)
{
    return ev->ident;
}

inline int
get_event(struct kevent *ev)
{
    return ev->filter;
}

inline void *
get_data(struct kevent *ev)
{
    return ev->udata;
}

inline int
is_eof(struct kevent *ev)
{
    return ev->flags & EV_EOF;
}
