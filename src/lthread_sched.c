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
 * lthread_sched.c
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <inttypes.h>

#include "lthread_int.h"
#include "lthread_time.h"
#include "tree.h"

#define FD_KEY(f,e) (((int64_t)(f) << (sizeof(int32_t) * 8)) | e)
#define FD_EVENT(f) ((int32_t)(f))
#define FD_ONLY(f) ((f) >> ((sizeof(int32_t) * 8)))

RB_GENERATE(lthread_rb_sleep, lthread, sleep_node, _lthread_sleep_cmp);
RB_GENERATE(lthread_rb_wait, lthread, wait_node, _lthread_wait_cmp);

static uint64_t _lthread_min_timeout(struct lthread_sched *);

static int  _lthread_poll(void);
static void _lthread_resume_expired(struct lthread_sched *sched);
static inline void _lthread_clear_rd_wr_state(struct lthread *lt);

static char tmp[100];
static struct lthread find_lt;

void
lthread_run(void)
{
    struct lthread_sched *sched;
    struct lthread *lt = NULL, *lt_tmp = NULL;
    struct lthread *lt_read = NULL, *lt_write = NULL;
    int p = 0;
    int fd = 0;
    int ret = 0;
    (void)ret; /* silence compiler */

    sched = lthread_get_sched();
    /* scheduler not initiliazed, and no lthreads where created*/
    if (sched == NULL)
        return;

    while (!RB_EMPTY(&sched->sleeping) ||
        !RB_EMPTY(&sched->waiting) ||
        !TAILQ_EMPTY(&sched->ready)) {

        /* 1. start by checking if a sleeping thread needs to wakeup */
        _lthread_resume_expired(sched);

        /* 2. check to see if we have any ready threads to run */
        while (!TAILQ_EMPTY(&sched->ready)) {
            TAILQ_FOREACH_SAFE(lt, &sched->ready, ready_next, lt_tmp) {
                TAILQ_REMOVE(&lt->sched->ready, lt, ready_next);
                _lthread_resume(lt);
            }
        }

        /* 3. resume lthreads we received from lthread_compute, if any */
        while (1) {
            pthread_mutex_lock(&sched->compute_mutex);
            lt = LIST_FIRST(&sched->compute);
            if (lt == NULL) {
                pthread_mutex_unlock(&sched->compute_mutex);
                break;
            }
            LIST_REMOVE(lt, compute_sched_next);
            assert(pthread_mutex_unlock(&sched->compute_mutex) == 0);
            _lthread_resume(lt);
        }

        /* 4. check if we received any events after lthread_poll */
        _lthread_poller_ev_register_rd(sched->compute_pipes[0]);
        _lthread_poll();

        /* 5. fire up lthreads that are ready to run */
        while (sched->num_new_events) {
            p = --sched->num_new_events;

            /* 
             * We got signaled via pipe to wakeup from polling & rusume compute.
             * Those lthreads will get handled in step 3.
             */
            fd = _lthread_poller_ev_get_fd(&sched->eventlist[p]);
            if (fd == sched->compute_pipes[0]) {
                ret = read(fd, &tmp, sizeof(tmp));
                assert(ret > 0);
                continue;
            }

            if (_lthread_poller_ev_is_eof(&sched->eventlist[p])) {
                lt->state |= BIT(LT_FDEOF);
                errno = ECONNRESET;
            }

            lt_read = _lthread_remove_waiting_on(fd, LT_READ);
            if (lt_read != NULL)
                _lthread_resume(lt_read);

            lt_write = _lthread_remove_waiting_on(fd, LT_WRITE);
            if (lt_write != NULL)
                _lthread_resume(lt_write);

            assert(lt_write != NULL || lt_read != NULL);
        }
    }

    _sched_free(sched);

    return;
}

struct lthread*
_lthread_remove_waiting_on(int fd, enum lthread_event e)
{
    struct lthread *lt = NULL;
    struct lthread_sched *sched = lthread_get_sched();
    find_lt.fd_wait = FD_KEY(fd, e);

    lt = RB_FIND(lthread_rb_wait, &sched->waiting, &find_lt);
    if (lt != NULL) {
        RB_REMOVE(lthread_rb_wait, &lt->sched->waiting, lt);
        _lthread_desched(lt);
        _lthread_clear_rd_wr_state(lt);
    }

    return (lt);
}

void
_lthread_wait_for(struct lthread *lt, int fd, enum lthread_event e)
{
    //printf("registering lt %llu for %d\n", lt->id, e);
    struct lthread *lt_tmp = NULL;
    if (lt->state & BIT(LT_WAIT_READ) ||
        lt->state & BIT(LT_WAIT_WRITE)) {
        printf("Unexpected event. lt id %"PRIu64" fd %ld already in %d state\n",
            lt->id, lt->fd_wait, lt->state);
        assert(0);
    }

    if (e == LT_READ)
        _lthread_poller_ev_register_rd(fd);
    else if (e == LT_WRITE)
        _lthread_poller_ev_register_wr(fd);
    else
        assert(0);

    lt->fd_wait = FD_KEY(fd, e);
    lt_tmp = RB_INSERT(lthread_rb_wait, &lt->sched->waiting, lt);
    assert(lt_tmp == NULL);
    _lthread_yield(lt);
}

static inline void
_lthread_clear_rd_wr_state(struct lthread *lt)
{
    if (lt->fd_wait >= 0) {
        //printf("%llu state is %d\n", lt->id, lt->state);
        if (lt->state & BIT(LT_WAIT_READ)) {
            lt->state &= CLEARBIT(LT_WAIT_READ);
        } else if (lt->state & BIT(LT_WAIT_WRITE)) {
            lt->state &= CLEARBIT(LT_WAIT_WRITE);
        } else {
            printf("lt->state is %d\n", lt->state);
            assert(0);
        }

        lt->fd_wait = -1;
    }
}

static int
_lthread_poll(void)
{
    struct lthread_sched *sched;
    struct timespec t = {0, 0};
    int ret = 0;
    uint64_t usecs = 0;

    sched = lthread_get_sched();

    sched->num_new_events = 0;
    usecs = _lthread_min_timeout(sched);

    /* never sleep if we have an lthread pending in the new queue */
    if (usecs && TAILQ_EMPTY(&sched->ready)) {
        t.tv_sec =  usecs / 1000000u;
        if (t.tv_sec != 0)
            t.tv_nsec  =  (usecs % 1000u)  * 1000000u;
        else
            t.tv_nsec = usecs * 1000u;
    }

    ret = _lthread_poller_poll(t);

    if (ret == -1) {
        perror("error adding events to kevent");
        assert(0);
    }

    sched->nevents = 0;
    sched->num_new_events = ret;

    return (0);
}

void
_lthread_desched(struct lthread *lt)
{
    struct lthread *lt_tmp = NULL;

    if (lt->state & BIT(LT_SLEEPING)) {
        RB_REMOVE(lthread_rb_sleep, &lt->sched->sleeping, lt_tmp);
        lt->state &= CLEARBIT(LT_SLEEPING);
        lt->state |= BIT(LT_READY);
        lt->state &= CLEARBIT(LT_EXPIRED);
    }
}

int
_lthread_sched(struct lthread *lt, uint64_t msecs)
{
    struct lthread *lt_tmp = NULL;
    uint64_t t_diff_usecs = 0;
    uint64_t usecs = msecs * 1000u;
    t_diff_usecs = tick_diff_usecs(lt->sched->birth, rdtsc()) + usecs;

    while (1) {
        /* resolve collision by adding usec until we find a non-existant node */
        lt->sleep_usecs = t_diff_usecs;
        lt_tmp = RB_INSERT(lthread_rb_sleep, &lt->sched->sleeping, lt);
        if (lt_tmp) {
            t_diff_usecs++;
            continue;
        }
        lt->state |= BIT(LT_SLEEPING);
        break;
    }

    return (0);
}

static uint64_t
_lthread_min_timeout(struct lthread_sched *sched)
{
    uint64_t t_diff_usecs = 0, min = 0;
    struct lthread *lt = NULL;

    t_diff_usecs = tick_diff_usecs(sched->birth, rdtsc());
    min = sched->default_timeout;

    lt = RB_MIN(lthread_rb_sleep, &sched->sleeping);
    if (!lt)
        return (min);

    min = lt->sleep_usecs;
    if (min > t_diff_usecs)
        return (min - t_diff_usecs);
    else // we are running late on a thread, execute immediately
        return (0);

    return (0);
}

static void
_lthread_resume_expired(struct lthread_sched *sched)
{
    struct lthread *lt = NULL;
    struct lthread *lt_tmp = NULL;
    uint64_t t_diff_usecs = 0;

    /* current scheduler time */
    t_diff_usecs = tick_diff_usecs(sched->birth, rdtsc());

    RB_FOREACH_SAFE(lt, lthread_rb_sleep, &sched->sleeping, lt_tmp) {
        if (lt->sleep_usecs <= t_diff_usecs) {
            if (lt->fd_wait >= 0) {
                /* lthread was waiting on an fd - remove it from tree */
                _lthread_remove_waiting_on(
                    FD_ONLY(lt->fd_wait), FD_EVENT(lt->fd_wait));

                /* remove event from poller */
                if (FD_EVENT(lt->fd_wait) == LT_WRITE)
                    _lthread_poller_ev_clear_wr(FD_ONLY(lt->fd_wait));
                else if (FD_EVENT(lt->fd_wait) == LT_READ)
                    _lthread_poller_ev_clear_rd(FD_ONLY(lt->fd_wait));
                else
                    assert(0); /* impossible state */

            } else {
                /* this lthread was just sleeping and it woke up */
                _lthread_desched(lt);
            }

            /* no need to clear expired if lthread exited/cancelled */
            if (_lthread_resume(lt) != -1)
                lt->state &= CLEARBIT(LT_EXPIRED);

            continue;
        }

        break;
    }
}
