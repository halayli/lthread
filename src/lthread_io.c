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

static void _lthread_io_add(struct lthread *lt);

static pthread_key_t io_worker_key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;


pthread_mutex_t io_workers_mutex = PTHREAD_MUTEX_INITIALIZER;

struct lthread_io_worker {
    struct lthread_l    lthreads;
    pthread_mutex_t     run_mutex;
    pthread_cond_t      run_mutex_cond;
    pthread_mutex_t     lthreads_mutex;
    enum lthread_io_st io_st;
};

static void *_lthread_io_worker(void *arg);

static int io_worker_running = 0;

static void
once_routine(void)
{
    assert(pthread_key_create(&io_worker_key, NULL) == 0);
}

int
_lthread_io_worker_init()
{

    if (io_worker_running > 0) {
        return (0);
    }

    /* XXX: create pthreads */
    static struct lthread_io_worker io_worker;
    pthread_t pthread;

    assert(pthread_mutex_init(&io_worker.lthreads_mutex, NULL) == 0);
    assert(pthread_mutex_init(&io_worker.run_mutex, NULL) == 0);

    io_worker_running = 1;

    assert(pthread_create(&pthread,
        NULL, _lthread_io_worker, &io_worker) == 0);

    return (0);
}

static void *
_lthread_io_worker(void *arg)
{
    struct lthread_io_worker *io_worker = arg;
    struct lthread *lt = NULL;
    struct timespec timeout;
    int status = 0;
    int ret = 0;
    (void)ret; /* silence compiler */

    assert(pthread_once(&key_once, once_routine) == 0);
    assert(pthread_setspecific(io_worker_key, arg) == 0);

    while (1) {

        /* resume lthreads to run their computation or make a blocking call */
        while (1) {
            assert(pthread_mutex_lock(&io_worker->lthreads_mutex) == 0);

            /* we have no work to do, break and wait 60 secs then exit */
            if (LIST_EMPTY(&io_worker->lthreads)) {
                assert(pthread_mutex_unlock(&io_worker->lthreads_mutex) == 0);
                break;
            }

            lt = LIST_FIRST(&io_worker->lthreads);
            LIST_REMOVE(lt, io_next);

            assert(pthread_mutex_unlock(&io_worker->lthreads_mutex) == 0);

            io_worker->io_st = LT_IO_BUSY;

            /* XXX: do read or write */
            if (lt->state & BIT(LT_ST_WAIT_IO_READ)) {
                lt->io.ret = read(lt->io.fd, lt->io.buf, lt->io.nbytes);
                lt->io.err = (lt->io.ret == -1) ? errno : 0;
            } else if (lt->state & BIT(LT_ST_WAIT_IO_WRITE)) {
                lt->io.ret = write(lt->io.fd, lt->io.buf, lt->io.nbytes);
                lt->io.err = (lt->io.ret == -1) ? errno : 0;
            } else
                assert(0);

            io_worker->io_st = LT_IO_FREE;

            /* resume it back on the  prev scheduler */
            assert(pthread_mutex_lock(&lt->sched->defer_mutex) == 0);
            TAILQ_INSERT_TAIL(&lt->sched->defer, lt, defer_next);
            assert(pthread_mutex_unlock(&lt->sched->defer_mutex) == 0);

            /* signal the prev scheduler in case it was sleeping in a poll */
            ret = write(lt->sched->io_pipes[1], "1", 1);
            assert(ret == 1);
        }

        assert(pthread_mutex_lock(&io_worker->run_mutex) == 0);
        timeout.tv_sec = time(NULL) + 60;
        timeout.tv_nsec = 0;
        status = pthread_cond_timedwait(&io_worker->run_mutex_cond,
            &io_worker->run_mutex, &timeout);
        assert(pthread_mutex_unlock(&io_worker->run_mutex) == 0);

        /* if we didn't timeout, then we got signaled to do some work */
        if (status != ETIMEDOUT)
            continue;

    }

}

static void
_lthread_io_add(struct lthread *lt)
{
    struct lthread_io_worker io_worker;

    LIST_INSERT_HEAD(&lt->sched->busy, lt, busy_next);

    assert(pthread_mutex_lock(&io_worker.run_mutex) == 0);

    LIST_INSERT_HEAD(&io_worker.lthreads, lt, io_next);

    /* wakeup pthread if it was sleeping */
    assert(pthread_cond_signal(&io_worker.run_mutex_cond) == 0);
    assert(pthread_mutex_unlock(&io_worker.run_mutex) == 0);

    _lthread_yield(lt);
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
