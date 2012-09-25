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

#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#include "lthread_poller.h"
#include "lthread_time.h"
#include "queue.h"
#include "tree.h"

#define LT_MAX_EVENTS    (1024)
#define MAX_STACK_SIZE (8*1024*1024)

#define BIT(x) (1 << (x))
#define CLEARBIT(x) ~(1 << (x))

struct lthread;
struct lthread_sched;
struct lthread_compute_sched;
struct lthread_cond;

inline uint64_t _lthread_sleep_cmp(struct lthread *l1, struct lthread *l2);
inline uint64_t _lthread_wait_cmp(struct lthread *l1, struct lthread *l2);

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
    COMPUTE_BUSY,
    COMPUTE_FREE,
};

enum lthread_st {
    LT_ST_WAIT_READ,   /* lthread waiting for READ on socket */
    LT_ST_WAIT_WRITE,  /* lthread waiting for WRITE on socket */
    LT_ST_NEW,         /* lthread spawned but needs initialization */
    LT_ST_READY,       /* lthread is ready to run */
    LT_ST_EXITED,      /* lthread has exited and needs cleanup */
    LT_ST_LOCKED,      /* lthread is locked on a condition */
    LT_ST_SLEEPING,    /* lthread is sleeping */
    LT_ST_EXPIRED,     /* lthread has expired and needs to run */
    LT_ST_FDEOF,       /* lthread socket has shut down */
    LT_ST_DETACH,      /* lthread frees when done, else it waits to join */
    LT_ST_CANCELLED,   /* lthread has been cancelled */
    LT_ST_RUNCOMPUTE,  /* lthread needs to run on a compute pthread */
    LT_ST_PENDING_RUNCOMPUTE, /* lthread needs to run on a compute pthread */
};

struct lthread {
    struct cpu_ctx          ctx;            /* cpu ctx info */
    lthread_func            fun;            /* func lthread is running */
    void                    *arg;           /* func args passed to func */
    void                    *data;          /* user ptr attached to lthread */
    size_t                  stack_size;     /* current stack_size */
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
    TAILQ_ENTRY(lthread)    ready_next;     /* ready to run list */
    TAILQ_ENTRY(lthread)    cond_next;      /* waiting on a cond var */
    LIST_ENTRY(lthread)     compute_next;   /* waiting to enter compute */
    LIST_ENTRY(lthread)     compute_sched_next; /* in compute scheduler */
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
    /* poller variables */
    int                 poller_fd;
#if defined(__FreeBSD__) || defined(__APPLE__)
    struct kevent       changelist[LT_MAX_EVENTS];
#endif
    POLL_EVENT_TYPE     eventlist[LT_MAX_EVENTS];
    int                 nevents;
    int                 num_new_events;
    int                 compute_pipes[2];
    pthread_mutex_t     compute_mutex;
    /* lists to save an lthread depending on its state */
    struct lthread_q        ready;
    struct lthread_l        compute;
    struct lthread_rb_sleep sleeping;
    struct lthread_rb_wait  waiting;
};

struct lthread_compute_sched {
    char                stack[MAX_STACK_SIZE];
    struct cpu_ctx      ctx;
    struct lthread_l    lthreads;
    struct lthread      *current_lthread;
    pthread_mutex_t     run_mutex;
    pthread_cond_t      run_mutex_cond;
    pthread_mutex_t     lthreads_mutex;
    LIST_ENTRY(lthread_compute_sched)    compute_next;
    enum lthread_compute_st compute_st;
};

int         sched_create(size_t stack_size);

int         _lthread_resume(struct lthread *lt);
inline void _lthread_renice(struct lthread *lt);
void        _sched_free(struct lthread_sched *sched);
void        _lthread_del_event(struct lthread *lt);

void        _lthread_yield(struct lthread *lt);
void        _lthread_free(struct lthread *lt);
void        _lthread_desched_sleep(struct lthread *lt);
void        _lthread_sched_sleep(struct lthread *lt, uint64_t msecs);
void        _lthread_cancel_event(struct lthread *lt);
struct lthread* _lthread_desched_event(int fd, enum lthread_event e);
void        _lthread_sched_event(struct lthread *lt, int fd,
    enum lthread_event e, uint64_t timeout);

inline int  _restore_exec_state(struct lthread *lt);
int         _switch(struct cpu_ctx *new_ctx, struct cpu_ctx *cur_ctx);
int         _save_exec_state(struct lthread *lt);
void        _lthread_compute_add(struct lthread *lt);

extern pthread_key_t lthread_sched_key;

static inline struct lthread_sched*
lthread_get_sched()
{
    return pthread_getspecific(lthread_sched_key);
}

#endif
