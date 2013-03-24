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
 * lthread_compute.c
 */

#include <sys/queue.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <pthread.h>

#include "lthread_int.h"

enum {THREAD_TIMEOUT_BEFORE_EXIT = 60};
static pthread_key_t compute_sched_key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

LIST_HEAD(compute_sched_l, lthread_compute_sched) compute_scheds = \
    LIST_HEAD_INITIALIZER(compute_scheds);
pthread_mutex_t sched_mutex = PTHREAD_MUTEX_INITIALIZER;

static void* _lthread_compute_run(void *arg);
static void _lthread_compute_resume(struct lthread *lt);
static struct lthread_compute_sched* _lthread_compute_sched_create(void);
static void _lthread_compute_sched_free(
    struct lthread_compute_sched *compute_sched);

struct lthread_compute_sched {
    struct cpu_ctx      ctx;
    struct lthread_q    lthreads;
    struct lthread      *current_lthread;
    pthread_mutex_t     run_mutex;
    pthread_cond_t      run_mutex_cond;
    pthread_mutex_t     lthreads_mutex;
    LIST_ENTRY(lthread_compute_sched)    compute_next;
    enum lthread_compute_st compute_st;
};

int
lthread_compute_begin(void)
{
    struct lthread_sched *sched = lthread_get_sched();
    struct lthread_compute_sched *compute_sched = NULL, *tmp = NULL;
    struct lthread *lt = sched->current_lthread;

    /* search for an empty compute_scheduler */
    assert(pthread_mutex_lock(&sched_mutex) == 0);
    LIST_FOREACH(tmp, &compute_scheds, compute_next) {
        if (tmp->compute_st == LT_COMPUTE_FREE) {
            compute_sched = tmp;
            break;
        }
    }

    /* create schedule if there is no scheduler available */
    if (compute_sched == NULL) {
        if ((compute_sched = _lthread_compute_sched_create()) == NULL) {
            /* we failed to create a scheduler. Use the first scheduler
             * in the list, otherwise return failure.
             */
            compute_sched = LIST_FIRST(&compute_scheds);
            if (compute_sched == NULL) {
                assert(pthread_mutex_unlock(&sched_mutex) == 0);
                return -1;
            }
        } else {
            LIST_INSERT_HEAD(&compute_scheds, compute_sched, compute_next);
        }
    }

    lt->compute_sched = compute_sched;

    lt->state |= BIT(LT_ST_PENDING_RUNCOMPUTE);
    assert(pthread_mutex_lock(&lt->compute_sched->lthreads_mutex) == 0);
    TAILQ_INSERT_TAIL(&lt->compute_sched->lthreads, lt, compute_next);
    assert(pthread_mutex_unlock(&lt->compute_sched->lthreads_mutex) == 0);

    assert(pthread_mutex_unlock(&sched_mutex) == 0);

    /* yield function in scheduler to allow other lthreads to run while
     * this lthread runs in a pthread for expensive computations.
     */
    _switch(&lt->sched->ctx, &lt->ctx);

    return (0);
}

void
lthread_compute_end(void)
{
    /* get current compute scheduler */
    struct lthread_compute_sched *compute_sched =
        pthread_getspecific(compute_sched_key);
    struct lthread *lt = compute_sched->current_lthread;
    assert(compute_sched != NULL);
    _switch(&compute_sched->ctx, &lt->ctx);
}

void
_lthread_compute_add(struct lthread *lt)
{

    LIST_INSERT_HEAD(&lt->sched->busy, lt, busy_next);
    /*
     * lthread is in scheduler list at this point. lock mutex to change
     * state since the state is checked in scheduler as well.
     */
    assert(pthread_mutex_lock(&lt->compute_sched->lthreads_mutex) == 0);
    lt->state &= CLEARBIT(LT_ST_PENDING_RUNCOMPUTE);
    lt->state |= BIT(LT_ST_RUNCOMPUTE);
    assert(pthread_mutex_unlock(&lt->compute_sched->lthreads_mutex) == 0);

    /* wakeup pthread if it was sleeping */
    assert(pthread_mutex_lock(&lt->compute_sched->run_mutex) == 0);
    assert(pthread_cond_signal(&lt->compute_sched->run_mutex_cond) == 0);
    assert(pthread_mutex_unlock(&lt->compute_sched->run_mutex) == 0);

}

static void
_lthread_compute_sched_free(struct lthread_compute_sched *compute_sched)
{
    assert(pthread_mutex_destroy(&compute_sched->run_mutex) == 0);
    assert(pthread_mutex_destroy(&compute_sched->lthreads_mutex) == 0);
    assert(pthread_cond_destroy(&compute_sched->run_mutex_cond) == 0);
    free(compute_sched);
}

static struct lthread_compute_sched*
_lthread_compute_sched_create(void)
{
    struct lthread_compute_sched *compute_sched = NULL;
    pthread_t pthread;

    if ((compute_sched = calloc(1,
        sizeof(struct lthread_compute_sched))) == NULL)
        return NULL;

    if (pthread_mutex_init(&compute_sched->run_mutex, NULL) != 0 ||
        pthread_mutex_init(&compute_sched->lthreads_mutex, NULL) != 0 ||
        pthread_cond_init(&compute_sched->run_mutex_cond, NULL) != 0) {
        free(compute_sched);
        return NULL;
    }

    if (pthread_create(&pthread,
        NULL, _lthread_compute_run, compute_sched) != 0) {
        _lthread_compute_sched_free(compute_sched);
        return NULL;
    }
    assert(pthread_detach(pthread) == 0);

    TAILQ_INIT(&compute_sched->lthreads);

    return compute_sched;
}

static void
_lthread_compute_resume(struct lthread *lt)
{
    _switch(&lt->ctx, &lt->compute_sched->ctx);
}

static void
once_routine(void)
{
    assert(pthread_key_create(&compute_sched_key, NULL) == 0);
}

static void*
_lthread_compute_run(void *arg)
{
    struct lthread_compute_sched *compute_sched = arg;
    struct lthread *lt = NULL;
    struct timespec timeout;
    int status = 0;
    int ret = 0;
    (void)ret; /* silence compiler */

    assert(pthread_once(&key_once, once_routine) == 0);
    assert(pthread_setspecific(compute_sched_key, arg) == 0);

    while (1) {

        /* resume lthreads to run their computation or make a blocking call */
        while (1) {
            assert(pthread_mutex_lock(&compute_sched->lthreads_mutex) == 0);

            /* we have no work to do, break and wait 60 secs then exit */
            if (TAILQ_EMPTY(&compute_sched->lthreads)) {
                assert(pthread_mutex_unlock(
                    &compute_sched->lthreads_mutex) == 0);
                break;
            }

            lt = TAILQ_FIRST(&compute_sched->lthreads);
            if (lt->state & BIT(LT_ST_PENDING_RUNCOMPUTE)) {
                assert(pthread_mutex_unlock(
                    &compute_sched->lthreads_mutex) == 0);
                continue;
            }

            TAILQ_REMOVE(&compute_sched->lthreads, lt, compute_next);

            assert(pthread_mutex_unlock(&compute_sched->lthreads_mutex) == 0);

            compute_sched->current_lthread = lt;
            compute_sched->compute_st = LT_COMPUTE_BUSY;

            _lthread_compute_resume(lt);

            compute_sched->current_lthread = NULL;
            compute_sched->compute_st = LT_COMPUTE_FREE;

            /* resume it back on the  prev scheduler */
            assert(pthread_mutex_lock(&lt->sched->defer_mutex) == 0);
            TAILQ_INSERT_TAIL(&lt->sched->defer, lt, defer_next);
            lt->state &= CLEARBIT(LT_ST_RUNCOMPUTE);
            assert(pthread_mutex_unlock(&lt->sched->defer_mutex) == 0);

            /* signal the prev scheduler in case it was sleeping in a poll */
            _lthread_poller_ev_trigger(lt->sched);
        }

        assert(pthread_mutex_lock(&compute_sched->run_mutex) == 0);
        /* wait if we have no work to do, exit */
        timeout.tv_sec = time(NULL) + THREAD_TIMEOUT_BEFORE_EXIT;
        timeout.tv_nsec = 0;
        status = pthread_cond_timedwait(&compute_sched->run_mutex_cond,
            &compute_sched->run_mutex, &timeout);
        assert(pthread_mutex_unlock(&compute_sched->run_mutex) == 0);

        /* if we didn't timeout, then we got signaled to do some work */
        if (status != ETIMEDOUT)
            continue;

        /* lock the global sched to check if we have any pending work to do */
        assert(pthread_mutex_lock(&sched_mutex) == 0);

        assert(pthread_mutex_lock(&compute_sched->lthreads_mutex) == 0);
        if (TAILQ_EMPTY(&compute_sched->lthreads)) {

            LIST_REMOVE(compute_sched, compute_next);

            assert(pthread_mutex_unlock(&compute_sched->lthreads_mutex) == 0);
            assert(pthread_mutex_unlock(&sched_mutex) == 0);
            _lthread_compute_sched_free(compute_sched);
            break;
        }

        assert(pthread_mutex_unlock(&compute_sched->lthreads_mutex) == 0);
        assert(pthread_mutex_unlock(&sched_mutex) == 0);
    }


    return NULL;
}
