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

  lthread_int.h
*/

#ifndef _LTHREAD_INT_H_
#define _LTHREAD_INT_H_ 

#include "common/queue.h"
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>

#if defined(__FreeBSD__) || defined(__APPLE__)
#include <sys/event.h>
#else
#include <sys/epoll.h>
#endif

#include <pthread.h>
#include <sys/time.h>
#include "common/time.h"
#include "common/rbtree.h"
#include "poller.h"

#define LT_MAX_EVENTS    (500)
#define MAX_STACK_SIZE (8*1024*1024)

#define MAX_FD 65535 * 2
#define MAX_CHANGELIST MAX_FD * 2

#define bit(x) (1 << (x))
#define clearbit(x) ~(1 << (x))

struct _lthread;
struct _sched;
struct _compute_sched;
struct _sched_node;
struct _lthread_cond;
typedef struct _lthread lthread_t;
typedef struct _lthread_cond lthread_cond_t;
typedef struct _sched sched_t;
typedef struct _compute_sched compute_sched_t;
typedef struct _sched_node sched_node_t;

LIST_HEAD(_sched_node_l, _sched_node);
typedef struct _sched_node_l _sched_node_l_t;
LIST_HEAD(_lthread_l, _lthread);
typedef struct _lthread_l lthread_l_t;

typedef enum {
    LT_READ,
    LT_WRITE,   
} lt_event_t;

struct _cpu_state {
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

typedef enum {
    LT_WAIT_READ,   /* lthread waiting for READ on socket */
    LT_WAIT_WRITE,  /* lthread waiting for WRITE on socket */
    LT_NEW,         /* lthread spawned but needs initialization */
    LT_READY,       /* lthread is ready to run */
    LT_EXITED,      /* lthread has exited and needs cleanup */
    LT_LOCKED,      /* lthread is locked on a condition */
    LT_SLEEPING,    /* lthread is sleeping */
    LT_EXPIRED,     /* lthread has expired and needs to run */
    LT_FDEOF,       /* lthread socket has shut down */
    LT_DETACH,      /* lthread will free once it's done, otherwise it waits for lthread_join */
    LT_RUNCOMPUTE, /* lthread needs to run on a compute pthread */
    LT_PENDING_RUNCOMPUTE, /* lthread needs to run on a compute pthread */
} lt_state_t; 

struct _sched_node {
    uint64_t usecs;
    struct rb_node node;
    lthread_l_t lthreads;
    LIST_ENTRY(_sched_node) next;
};

struct _lthread {
    struct              _cpu_state st;
    void                (*fun)(lthread_t *lt, void *);
    void                *arg;
    void                *data;
    size_t              stack_size;
    lt_state_t          state; 
    sched_t             *sched;
    compute_sched_t     *compute_sched;
    uint64_t            timeout;
    uint64_t            ticks;
    uint64_t            birth;
    uint64_t            id;
    int                 fd_wait; /* fd we are waiting on */
    char                funcname[64];
    lthread_t           *lt_join;
    void                **lt_exit_ptr;
    LIST_ENTRY(_lthread)    new_next;
    LIST_ENTRY(_lthread)    sleep_next;
    LIST_ENTRY(_lthread)    compute_next;
    LIST_ENTRY(_lthread)    compute_sched_next;
    sched_node_t        *sched_node;
    lthread_l_t         *sleep_list;
    void                *stack;
    void                *ebp;
};

struct _lthread_cond {
    lthread_t *blocked_lthread;
};

struct _sched {
    size_t              stack_size;
    int                 total_lthreads;
    int                 waiting_state;
    int                 sleeping_state;
    int                 poller;
    int                 nevents;
    uint64_t            default_timeout;
    int                 total_new_events;
    /* lists to save an lthread depending on its state */
    lthread_l_t         new;
    lthread_l_t         compute;
    struct rb_root      sleeping;
    uint64_t            birth;
    void                *stack;
    lthread_t           *current_lthread;
    struct _cpu_state   st;
#if defined(__FreeBSD__) || defined(__APPLE__)
    struct kevent       changelist[MAX_CHANGELIST];
    struct kevent       eventlist[LT_MAX_EVENTS];
#else
    struct epoll_event  eventlist[LT_MAX_EVENTS];
#endif
    int                 compute_pipes[2];
    pthread_mutex_t     compute_mutex;
};

typedef enum {
    COMPUTE_BUSY,
    COMPUTE_FREE,
} compute_state_t;

struct _compute_sched {
    char                stack[MAX_STACK_SIZE];
    struct _cpu_state   st;
    lthread_l_t         lthreads;
    lthread_t           *current_lthread;
    pthread_mutex_t     run_mutex;
    pthread_cond_t      run_mutex_cond;
    pthread_mutex_t     lthreads_mutex;
    LIST_ENTRY(_compute_sched)    compute_next;
    compute_state_t     state;
};

int     _lthread_resume(lthread_t *lt);
void    _sched_free(sched_t *sched);
void    _lthread_del_event(lthread_t *lt);

void    _lthread_yield(lthread_t *lt);
void    _lthread_free(lthread_t *lt);
void    _lthread_wait_for(lthread_t *lt, int fd, lt_event_t e);
int     _sched_lthread(lthread_t *lt,  uint64_t usecs);
void    _desched_lthread(lthread_t *lt);
void    clear_rd_wr_state(lthread_t *lt);
inline int _restore_exec_state(lthread_t *lt);
int     _switch(struct _cpu_state *new_state, struct _cpu_state *cur_state);
int     sched_create(size_t stack_size);
int     _save_exec_state(lthread_t *lt);

void    _lthread_compute_add(lthread_t *lt);

extern pthread_key_t lthread_sched_key;

static inline sched_t *
lthread_get_sched()
{
    return pthread_getspecific(lthread_sched_key);
}

#endif
