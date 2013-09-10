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
 * lthread_epoll.c
 */

#include "lthread_int.h"
#include <assert.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>

int
_lthread_poller_create(void)
{
    return (epoll_create(1024));
}

inline int
_lthread_poller_poll(struct timespec t)
{
    struct lthread_sched *sched = lthread_get_sched();

    return (epoll_wait(sched->poller_fd, sched->eventlist, LT_MAX_EVENTS,
        t.tv_sec*1000.0 + t.tv_nsec/1000000.0));
}

inline void
_lthread_poller_ev_clear_rd(int fd)
{
    struct epoll_event ev;
    int ret = 0;
    struct lthread_sched *sched = lthread_get_sched();

    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP;
    ret = epoll_ctl(sched->poller_fd, EPOLL_CTL_DEL, fd, &ev);
    assert(ret != -1);
}

inline void
_lthread_poller_ev_clear_wr(int fd)
{
    struct epoll_event ev;
    int ret = 0;
    struct lthread_sched *sched = lthread_get_sched();

    ev.data.fd = fd;
    ev.events = EPOLLOUT | EPOLLONESHOT | EPOLLRDHUP;
    ret = epoll_ctl(sched->poller_fd, EPOLL_CTL_DEL, fd, &ev);
    assert(ret != -1);
}

inline void
_lthread_poller_ev_register_rd(int fd)
{
    struct epoll_event ev;
    int ret = 0;
    struct lthread_sched *sched = lthread_get_sched();

    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP;
    ev.data.fd = fd;
    ret = epoll_ctl(sched->poller_fd, EPOLL_CTL_MOD, fd, &ev);
    if (ret < 0)
        ret = epoll_ctl(sched->poller_fd, EPOLL_CTL_ADD, fd, &ev);
    assert(ret != -1);
}

inline void
_lthread_poller_ev_register_wr(int fd)
{
    struct epoll_event ev;
    int ret = 0;
    struct lthread_sched *sched = lthread_get_sched();

    ev.events = EPOLLOUT | EPOLLONESHOT | EPOLLRDHUP;
    ev.data.fd = fd;
    ret = epoll_ctl(sched->poller_fd, EPOLL_CTL_MOD, fd, &ev);
    if (ret < 0)
        ret = epoll_ctl(sched->poller_fd, EPOLL_CTL_ADD, fd, &ev);
    assert(ret != -1);
}

inline int
_lthread_poller_ev_get_fd(struct epoll_event *ev)
{
    return (ev->data.fd);
}

inline int
_lthread_poller_ev_get_event(struct epoll_event *ev)
{
    return (ev->events);
}

inline int
_lthread_poller_ev_is_eof(struct epoll_event *ev)
{
    return (ev->events & EPOLLHUP);
}

inline int
_lthread_poller_ev_is_write(struct epoll_event *ev)
{
    return (ev->events & EPOLLOUT);
}

inline int
_lthread_poller_ev_is_read(struct epoll_event *ev)
{
    return (ev->events & EPOLLIN);
}

inline void
_lthread_poller_ev_register_trigger(void)
{
    struct lthread_sched *sched = lthread_get_sched();
    int ret = 0;
    struct epoll_event ev;

    if (!sched->eventfd) {
        sched->eventfd = eventfd(0, EFD_NONBLOCK);
        assert(sched->eventfd != -1);
    }
    ev.events = EPOLLIN;
    ev.data.fd = sched->eventfd;
    ret = epoll_ctl(sched->poller_fd, EPOLL_CTL_ADD, sched->eventfd, &ev);
    assert(ret != -1);
}

inline void
_lthread_poller_ev_clear_trigger(void)
{
    uint64_t tmp;
    struct lthread_sched *sched = lthread_get_sched();
    assert(read(sched->eventfd, &tmp, sizeof(uint64_t)) == sizeof(uint64_t));
}

inline void
_lthread_poller_ev_trigger(struct lthread_sched *sched)
{
    uint64_t tmp = 2;
    assert(write(sched->eventfd, &tmp, sizeof(uint64_t)) == sizeof(uint64_t));
}
