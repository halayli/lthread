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

  epoll.c
*/

#include "lthread_int.h"
#include <assert.h>

void
register_rd_interest(int fd)
{
    struct epoll_event ev;
    int ret = 0;

    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP;
    ev.data.fd = fd;
    ev.data.ptr = sched->current_lthread;
    ret = epoll_ctl(sched->poller, EPOLL_CTL_MOD, fd, &ev);
    if (ret < 0)
        ret = epoll_ctl(sched->poller, EPOLL_CTL_ADD, fd, &ev);
    assert(ret != -1);

    sched->current_lthread->state |= bit(LT_WAIT_READ);
}

void
register_wr_interest(int fd)
{
    struct epoll_event ev;
    int ret = 0;

    ev.events = EPOLLOUT | EPOLLONESHOT | EPOLLRDHUP;
    ev.data.fd = fd;
    ev.data.ptr = sched->current_lthread;
    ret = epoll_ctl(sched->poller, EPOLL_CTL_MOD, fd, &ev);
    if (ret < 0)
        ret = epoll_ctl(sched->poller, EPOLL_CTL_ADD, fd, &ev);
    assert(ret != -1);

    sched->current_lthread->state |= bit(LT_WAIT_WRITE);
}

void
clear_interest(int fd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    epoll_ctl(sched->poller, EPOLL_CTL_DEL, fd, &ev);
}

int
create_poller(void)
{
    return epoll_create(1024);
}

int
poll_events(struct timespec t)
{
    return epoll_wait(sched->poller, sched->eventlist, LT_MAX_EVENTS,
        t.tv_sec*1000 + t.tv_nsec/1000000);
}

int
get_fd(struct epoll_event *ev)
{
    return ev->data.fd;
}

int
get_event(struct epoll_event *ev)
{
    return ev->events;
}

void *
get_data(struct epoll_event *ev)
{
    return ev->data.ptr;
}

inline int
is_eof(struct epoll_event *ev)
{
    return ev->events & EPOLLHUP;
}
