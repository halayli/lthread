/*
  memqueue
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

  kqueue.c
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
