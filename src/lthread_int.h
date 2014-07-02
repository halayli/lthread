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
 * lthread_int.c
 */


#ifndef LTHREAD_INT_H
#define LTHREAD_INT_H

#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#include "lthread_poller.h"
#include "queue.h"
#include "tree.h"

#define LT_MAX_EVENTS    (1024)
#define MAX_STACK_SIZE (128*1024) /* 128k */

#define BIT(x) (1 << (x))
#define CLEARBIT(x) ~(1 << (x))

struct lthread;
struct lthread_sched;
struct lthread_compute_sched;
struct lthread_io_sched;
struct lthread_cond;

LIST_HEAD(lthread_l, lthread);
TAILQ_HEAD(lthread_q, lthread);

typedef void (*lthread_func)(void *);

struct cpu_ctx {
    void     *esp;
    void     *ebp;
    void     *eip;
    void     *edi;
    void     *esi;
    void     *ebx;
    void     *r1;
    void     *r2;
    void     *r3;
    void     *r4;
    void     *r5;
};

enum lthread_event {
    LT_EV_READ,
    LT_EV_WRITE
};

enum lthread_compute_st {
    LT_COMPUTE_BUSY,
    LT_COMPUTE_FREE,
};

enum lthread_st {
    LT_ST_WAIT_READ,    /* lthread waiting for READ on socket */
    LT_ST_WAIT_WRITE,   /* lthread waiting for WRITE on socket */
    LT_ST_NEW,          /* lthread spawned but needs initialization */
    LT_ST_READY,        /* lthread is ready to run */
    LT_ST_EXITED,       /* lthread has exited and needs cleanup */
    LT_ST_BUSY,         /* lthread is waiting on join/cond/compute/io */
    LT_ST_SLEEPING,     /* lthread is sleeping */
    LT_ST_EXPIRED,      /* lthread has expired and needs to run */
    LT_ST_FDEOF,        /* lthread socket has shut down */
    LT_ST_DETACH,       /* lthread frees when done, else it waits to join */
    LT_ST_CANCELLED,    /* lthread has been cancelled */
    LT_ST_PENDING_RUNCOMPUTE, /* lthread needs to run in compute sched, step1 */
    LT_ST_RUNCOMPUTE,   /* lthread needs to run in compute sched (2), step2 */
    LT_ST_WAIT_IO_READ, /* lthread waiting for READ IO to finish */
    LT_ST_WAIT_IO_WRITE /* lthread waiting for WRITE IO to finish */
};

struct lthread {
    struct cpu_ctx          ctx;            /* cpu ctx info */
    lthread_func            fun;            /* func lthread is running */
    void                    *arg;           /* func args passed to func */
    void                    *data;          /* user ptr attached to lthread */
    size_t                  stack_size;     /* current stack_size */
    size_t                  last_stack_size; /* last yield  stack_size */
    enum lthread_st         state;          /* current lthread state */
    struct lthread_sched    *sched;         /* scheduler lthread belongs to */
    uint64_t                birth;          /* time lthread was born */
    uint64_t                id;             /* lthread id */
    int64_t                 fd_wait;        /* fd we are waiting on */
    char                    funcname[64];   /* optional func name */
    struct lthread          *lt_join;       /* lthread we want to join on */
    void                    **lt_exit_ptr;  /* exit ptr for lthread_join */
    void                    *stack;         /* ptr to lthread_stack */
    void                    *ebp;           /* saved for compute sched */
    uint32_t                ops;            /* num of ops since yield */
    uint64_t                sleep_usecs;    /* how long lthread is sleeping */
    RB_ENTRY(lthread)       sleep_node;     /* sleep tree node pointer */
    RB_ENTRY(lthread)       wait_node;      /* event tree node pointer */
    LIST_ENTRY(lthread)     busy_next;      /* blocked lthreads */
    TAILQ_ENTRY(lthread)    ready_next;     /* ready to run list */
    TAILQ_ENTRY(lthread)    defer_next;     /* ready to run after deferred job */
    TAILQ_ENTRY(lthread)    cond_next;      /* waiting on a cond var */
    TAILQ_ENTRY(lthread)    io_next;        /* waiting its turn in io */
    TAILQ_ENTRY(lthread)    compute_next;   /* waiting to run in compute sched */
    struct {
        void *buf;
        size_t nbytes;
        int fd;
        int ret;
        int err;
    } io;
    /* lthread_compute schduler - when running in compute block */
    struct lthread_compute_sched    *compute_sched;
};

RB_HEAD(lthread_rb_sleep, lthread);
RB_HEAD(lthread_rb_wait, lthread);
RB_PROTOTYPE(lthread_rb_wait, lthread, wait_node, _lthread_wait_cmp);

struct lthread_cond {
    struct lthread_q blocked_lthreads;
};

struct lthread_sched {
    uint64_t            birth;
    struct cpu_ctx      ctx;
    void                *stack;
    size_t              stack_size;
    int                 spawned_lthreads;
    uint64_t            default_timeout;
    struct lthread      *current_lthread;
    int                 page_size;
    /* poller variables */
    int                 poller_fd;
#if defined(__FreeBSD__) || defined(__APPLE__)
    struct kevent       changelist[LT_MAX_EVENTS];
#endif
    int                 eventfd;
    POLL_EVENT_TYPE     eventlist[LT_MAX_EVENTS];
    int                 nevents;
    int                 num_new_events;
    pthread_mutex_t     defer_mutex;
    /* lists to save an lthread depending on its state */
    /* lthreads ready to run */
    struct lthread_q        ready;
    /* lthreads ready to run after io or compute is done */
    struct lthread_q        defer;
    /* lthreads in join/cond_wait/io/compute */
    struct lthread_l        busy;
    /* lthreads zzzzz */
    struct lthread_rb_sleep sleeping;
    /* lthreads waiting on socket io */
    struct lthread_rb_wait  waiting;
};


int         sched_create(size_t stack_size);

int         _lthread_resume(struct lthread *lt);
void _lthread_renice(struct lthread *lt);
void        _sched_free(struct lthread_sched *sched);
void        _lthread_del_event(struct lthread *lt);

void        _lthread_yield(struct lthread *lt);
void        _lthread_free(struct lthread *lt);
void        _lthread_desched_sleep(struct lthread *lt);
void        _lthread_sched_sleep(struct lthread *lt, uint64_t msecs);
void        _lthread_sched_busy_sleep(struct lthread *lt, uint64_t msecs);
void        _lthread_cancel_event(struct lthread *lt);
struct lthread* _lthread_desched_event(int fd, enum lthread_event e);
void        _lthread_sched_event(struct lthread *lt, int fd,
    enum lthread_event e, uint64_t timeout);

int         _switch(struct cpu_ctx *new_ctx, struct cpu_ctx *cur_ctx);
int         _save_exec_state(struct lthread *lt);
void        _lthread_compute_add(struct lthread *lt);
void         _lthread_io_worker_init();

extern pthread_key_t lthread_sched_key;
void print_timestamp(char *);

static inline struct lthread_sched*
lthread_get_sched()
{
    return pthread_getspecific(lthread_sched_key);
}

static inline uint64_t
_lthread_diff_usecs(uint64_t t1, uint64_t t2)
{
    return (t2 - t1);
}

static inline uint64_t
_lthread_usec_now(void)
{
    struct timeval t1 = {0, 0};
    gettimeofday(&t1, NULL);
    return (t1.tv_sec * 1000000) + t1.tv_usec;
}

#endif
