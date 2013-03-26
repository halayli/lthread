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
 * lthread_io.c
 */

#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "lthread_int.h"

#define IO_WORKERS 2

static void _lthread_io_add(struct lthread *lt);
static void *_lthread_io_worker(void *arg);

static pthread_once_t key_once = PTHREAD_ONCE_INIT;

struct lthread_io_worker {
    struct lthread_q    lthreads;
    pthread_mutex_t     run_mutex;
    pthread_cond_t      run_mutex_cond;
    pthread_mutex_t     lthreads_mutex;
};

static struct lthread_io_worker io_workers[IO_WORKERS];

static void
once_routine(void)
{
    pthread_t pthread;
    struct lthread_io_worker *io_worker = NULL;
    int i = 0;

    for (i = 0; i < IO_WORKERS; i++) {
        io_worker = &io_workers[i];

        assert(pthread_mutex_init(&io_worker->lthreads_mutex, NULL) == 0);
        assert(pthread_mutex_init(&io_worker->run_mutex, NULL) == 0);
        assert(pthread_create(&pthread,
            NULL, _lthread_io_worker, io_worker) == 0);
        TAILQ_INIT(&io_worker->lthreads);

    }
}

void
_lthread_io_worker_init()
{
    assert(pthread_once(&key_once, once_routine) == 0);
}

static void *
_lthread_io_worker(void *arg)
{
    struct lthread_io_worker *io_worker = arg;
    struct lthread *lt = NULL;

    assert(pthread_once(&key_once, once_routine) == 0);

    while (1) {

        while (1) {
            assert(pthread_mutex_lock(&io_worker->lthreads_mutex) == 0);

            /* we have no work to do, break and wait */
            if (TAILQ_EMPTY(&io_worker->lthreads)) {
                assert(pthread_mutex_unlock(&io_worker->lthreads_mutex) == 0);
                break;
            }

            lt = TAILQ_FIRST(&io_worker->lthreads);
            TAILQ_REMOVE(&io_worker->lthreads, lt, io_next);

            assert(pthread_mutex_unlock(&io_worker->lthreads_mutex) == 0);

            if (lt->state & BIT(LT_ST_WAIT_IO_READ)) {
                lt->io.ret = read(lt->io.fd, lt->io.buf, lt->io.nbytes);
                lt->io.err = (lt->io.ret == -1) ? errno : 0;
            } else if (lt->state & BIT(LT_ST_WAIT_IO_WRITE)) {
                lt->io.ret = write(lt->io.fd, lt->io.buf, lt->io.nbytes);
                lt->io.err = (lt->io.ret == -1) ? errno : 0;
            } else
                assert(0);

            /* resume it back on the  prev scheduler */
            assert(pthread_mutex_lock(&lt->sched->defer_mutex) == 0);
            TAILQ_INSERT_TAIL(&lt->sched->defer, lt, defer_next);
            assert(pthread_mutex_unlock(&lt->sched->defer_mutex) == 0);

            /* signal the prev scheduler in case it was sleeping in a poll */
            _lthread_poller_ev_trigger(lt->sched);
        }

        assert(pthread_mutex_lock(&io_worker->run_mutex) == 0);
        pthread_cond_wait(&io_worker->run_mutex_cond,
            &io_worker->run_mutex);
        assert(pthread_mutex_unlock(&io_worker->run_mutex) == 0);

    }

}

static void
_lthread_io_add(struct lthread *lt)
{
    static uint32_t io_selector = 0;
    struct lthread_io_worker *io_worker = &io_workers[io_selector++];
    io_selector = io_selector % IO_WORKERS;

    LIST_INSERT_HEAD(&lt->sched->busy, lt, busy_next);

    assert(pthread_mutex_lock(&io_worker->lthreads_mutex) == 0);
    TAILQ_INSERT_TAIL(&io_worker->lthreads, lt, io_next);
    assert(pthread_mutex_unlock(&io_worker->lthreads_mutex) == 0);

    /* wakeup pthread if it was sleeping */
    assert(pthread_mutex_lock(&io_worker->run_mutex) == 0);
    assert(pthread_cond_signal(&io_worker->run_mutex_cond) == 0);
    assert(pthread_mutex_unlock(&io_worker->run_mutex) == 0);

    _lthread_yield(lt);

    /* restore errno we got from io worker, if any */
    if (lt->io.ret == -1)
        errno = lt->io.err;
}

ssize_t
lthread_io_read(int fd, void *buf, size_t nbytes)
{
    struct lthread *lt = lthread_get_sched()->current_lthread;
    lt->state |= BIT(LT_ST_WAIT_IO_READ);
    lt->io.buf = buf;
    lt->io.fd = fd;
    lt->io.nbytes = nbytes;

    _lthread_io_add(lt);
    lt->state &= CLEARBIT(LT_ST_WAIT_IO_READ);

    return (lt->io.ret);
}

ssize_t
lthread_io_write(int fd, void *buf, size_t nbytes)
{
    struct lthread *lt = lthread_get_sched()->current_lthread;
    lt->state |= BIT(LT_ST_WAIT_IO_WRITE);
    lt->io.buf = buf;
    lt->io.nbytes = nbytes;
    lt->io.fd = fd;

    _lthread_io_add(lt);
    lt->state &= CLEARBIT(LT_ST_WAIT_IO_WRITE);

    return (lt->io.ret);
}
