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
 * poller.c
 */


#if defined(__FreeBSD__) || defined(__APPLE__)
#include "lthread_kqueue.c"
#else
#include "lthread_epoll.c"
#endif

#include "lthread_int.h"

void
_lthread_poller_set_fd_ready(struct lthread *lt, int fd, enum lthread_event e,
    int is_eof)
{
    /* 
     * not all scheduled fds in the poller are guaranteed to have triggered,
     * deschedule them all and cancel events in poller so they don't trigger later.
     */
    int i;
    if (lt->ready_fds == 0) {
        for (i = 0; i < lt->nfds; i++)
            if (lt->pollfds[i].events & POLLIN) {
                _lthread_poller_ev_clear_rd(lt->pollfds[i].fd);
                _lthread_desched_event(lt->pollfds[i].fd, LT_EV_READ);
            } else if (lt->pollfds[i].events & POLLOUT) {
                _lthread_poller_ev_clear_wr(lt->pollfds[i].fd);
                _lthread_desched_event(lt->pollfds[i].fd, LT_EV_WRITE);
            }
    }


    lt->pollfds[lt->ready_fds].fd = fd;
    if (e == LT_EV_WRITE)
        lt->pollfds[lt->ready_fds].events = POLLOUT;
    else
        lt->pollfds[lt->ready_fds].events = POLLIN;

    if (is_eof)
        lt->pollfds[lt->ready_fds].events |= POLLHUP;

    lt->ready_fds++;
}
