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
 * lthread.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "lthread_int.h"
#include "lthread_poller.h"

extern int errno;

static void _exec(void *lt);
static void _lthread_init(struct lthread *lt);
static void _lthread_key_create(void);
static inline void _lthread_madvise(struct lthread *lt);

pthread_key_t lthread_sched_key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;



int _switch(struct cpu_ctx *new_ctx, struct cpu_ctx *cur_ctx);
#ifdef __i386__
__asm__ (
"    .text                                  \n"
"    .p2align 2,,3                          \n"
".globl _switch                             \n"
"_switch:                                   \n"
"__switch:                                  \n"
"movl 8(%esp), %edx      # fs->%edx         \n"
"movl %esp, 0(%edx)      # save esp         \n"
"movl %ebp, 4(%edx)      # save ebp         \n"
"movl (%esp), %eax       # save eip         \n"
"movl %eax, 8(%edx)                         \n"
"movl %ebx, 12(%edx)     # save ebx,esi,edi \n"
"movl %esi, 16(%edx)                        \n"
"movl %edi, 20(%edx)                        \n"
"movl 4(%esp), %edx      # ts->%edx         \n"
"movl 20(%edx), %edi     # restore ebx,esi,edi      \n"
"movl 16(%edx), %esi                                \n"
"movl 12(%edx), %ebx                                \n"
"movl 0(%edx), %esp      # restore esp              \n"
"movl 4(%edx), %ebp      # restore ebp              \n"
"movl 8(%edx), %eax      # restore eip              \n"
"movl %eax, (%esp)                                  \n"
"ret                                                \n"
);
#elif defined(__x86_64__)

__asm__ (
"    .text                                  \n"
"       .p2align 4,,15                                   \n"
".globl _switch                                          \n"
".globl __switch                                         \n"
"_switch:                                                \n"
"__switch:                                               \n"
"       movq %rsp, 0(%rsi)      # save stack_pointer     \n"
"       movq %rbp, 8(%rsi)      # save frame_pointer     \n"
"       movq (%rsp), %rax       # save insn_pointer      \n"
"       movq %rax, 16(%rsi)                              \n"
"       movq %rbx, 24(%rsi)     # save rbx,r12-r15       \n"
"       movq %r12, 32(%rsi)                              \n"
"       movq %r13, 40(%rsi)                              \n"
"       movq %r14, 48(%rsi)                              \n"
"       movq %r15, 56(%rsi)                              \n"
"       movq 56(%rdi), %r15                              \n"
"       movq 48(%rdi), %r14                              \n"
"       movq 40(%rdi), %r13     # restore rbx,r12-r15    \n"
"       movq 32(%rdi), %r12                              \n"
"       movq 24(%rdi), %rbx                              \n"
"       movq 8(%rdi), %rbp      # restore frame_pointer  \n"
"       movq 0(%rdi), %rsp      # restore stack_pointer  \n"
"       movq 16(%rdi), %rax     # restore insn_pointer   \n"
"       movq %rax, (%rsp)                                \n"
"       ret                                              \n"
);
#endif

static void
_exec(void *lt)
{

#if defined(__llvm__) && defined(__x86_64__)
  __asm__ ("movq 16(%%rbp), %[lt]" : [lt] "=r" (lt));
#endif
    ((struct lthread *)lt)->fun(((struct lthread *)lt)->arg);
    ((struct lthread *)lt)->state |= BIT(LT_ST_EXITED);

    _lthread_yield(lt);
}

void
_lthread_yield(struct lthread *lt)
{
    lt->ops = 0;
    _switch(&lt->sched->ctx, &lt->ctx);
}

void
_lthread_free(struct lthread *lt)
{
    free(lt->stack);
    free(lt);
}

int
_lthread_resume(struct lthread *lt)
{

    struct lthread_sched *sched = lthread_get_sched();

    if (lt->state & BIT(LT_ST_CANCELLED)) {
        /* if an lthread was joining on it, schedule it to run */
        if (lt->lt_join) {
            _lthread_desched_sleep(lt->lt_join);
            TAILQ_INSERT_TAIL(&sched->ready, lt->lt_join, ready_next);
            lt->lt_join = NULL;
        }
        /* if lthread is detached, then we can free it up */
        if (lt->state & BIT(LT_ST_DETACH))
            _lthread_free(lt);
        if (lt->state & BIT(LT_ST_BUSY))
            LIST_REMOVE(lt, busy_next);
        return (-1);
    }

    if (lt->state & BIT(LT_ST_NEW))
        _lthread_init(lt);

    sched->current_lthread = lt;
    _switch(&lt->ctx, &lt->sched->ctx);
    sched->current_lthread = NULL;
    _lthread_madvise(lt);

    if (lt->state & BIT(LT_ST_EXITED)) {
        if (lt->lt_join) {
            /* if lthread was sleeping, deschedule it so it doesn't expire. */
            _lthread_desched_sleep(lt->lt_join);
            TAILQ_INSERT_TAIL(&sched->ready, lt->lt_join, ready_next);
            lt->lt_join = NULL;
        }

        /* if lthread is detached, free it, otherwise lthread_join() will */
        if (lt->state & BIT(LT_ST_DETACH))
            _lthread_free(lt);
        return (-1);
    } else {
        /* place it in a compute scheduler if needed. */
        if (lt->state & BIT(LT_ST_PENDING_RUNCOMPUTE)) {
            _lthread_compute_add(lt);
        }
    }

    return (0);
}

static inline void
_lthread_madvise(struct lthread *lt)
{
    size_t current_stack = (lt->stack + lt->stack_size) - lt->ctx.esp;
    size_t tmp;
    /* make sure function did not overflow stack, we can't recover from that */
    assert(current_stack <= lt->stack_size);

    /* 
     * free up stack space we no longer use. As long as we were using more than
     * pagesize bytes.
     */
    if (current_stack < lt->last_stack_size &&
        lt->last_stack_size > lt->sched->page_size) {
        /* round up to the nearest page size */
        tmp = current_stack + (-current_stack & (lt->sched->page_size - 1));
        assert(madvise(lt->stack, lt->stack_size - tmp, MADV_DONTNEED) == 0);
    }

    lt->last_stack_size = current_stack;
}

static void
_lthread_key_destructor(void *data)
{
    free(data);
}

static void
_lthread_key_create(void)
{
    assert(pthread_key_create(&lthread_sched_key,
        _lthread_key_destructor) == 0);
    assert(pthread_setspecific(lthread_sched_key, NULL) == 0);

    return;
}

int
lthread_init(size_t size)
{
    return (sched_create(size));
}

static void
_lthread_init(struct lthread *lt)
{
    void **stack = NULL;
    stack = (void **)(lt->stack + (lt->stack_size));

    stack[-3] = NULL;
    stack[-2] = (void *)lt;
    lt->ctx.esp = (void *)stack - (4 * sizeof(void *));
    lt->ctx.ebp = (void *)stack - (3 * sizeof(void *));
    lt->ctx.eip = (void *)_exec;
    lt->state = BIT(LT_ST_READY);
}

void
_sched_free(struct lthread_sched *sched)
{
    close(sched->poller_fd);

#if ! (defined(__FreeBSD__) && defined(__APPLE__))
    close(sched->eventfd);
#endif
    pthread_mutex_destroy(&sched->defer_mutex);

    free(sched);
    pthread_setspecific(lthread_sched_key, NULL);
}

int
sched_create(size_t stack_size)
{
    struct lthread_sched *new_sched;
    size_t sched_stack_size = 0;

    sched_stack_size = stack_size ? stack_size : MAX_STACK_SIZE;

    if ((new_sched = calloc(1, sizeof(struct lthread_sched))) == NULL) {
        perror("Failed to initialize scheduler\n");
        return (errno);
    }

    assert(pthread_setspecific(lthread_sched_key, new_sched) == 0);
    _lthread_io_worker_init();

    if ((new_sched->poller_fd = _lthread_poller_create()) == -1) {
        perror("Failed to initialize poller\n");
        _sched_free(new_sched);
        return (errno);
    }
    _lthread_poller_ev_register_trigger();

    if (pthread_mutex_init(&new_sched->defer_mutex, NULL) != 0) {
        perror("Failed to initialize defer_mutex\n");
        _sched_free(new_sched);
        return (errno);
    }

    new_sched->stack_size = sched_stack_size;
    new_sched->page_size = getpagesize();

    new_sched->spawned_lthreads = 0;
    new_sched->default_timeout = 3000000u;
    RB_INIT(&new_sched->sleeping);
    RB_INIT(&new_sched->waiting);
    new_sched->birth = _lthread_usec_now();
    TAILQ_INIT(&new_sched->ready);
    TAILQ_INIT(&new_sched->defer);
    LIST_INIT(&new_sched->busy);

    bzero(&new_sched->ctx, sizeof(struct cpu_ctx));

    return (0);
}

int
lthread_create(struct lthread **new_lt, void *fun, void *arg)
{
    struct lthread *lt = NULL;
    assert(pthread_once(&key_once, _lthread_key_create) == 0);
    struct lthread_sched *sched = lthread_get_sched();

    if (sched == NULL) {
        sched_create(0);
        sched = lthread_get_sched();
        if (sched == NULL) {
            perror("Failed to create scheduler");
            return (-1);
        }
    }

    if ((lt = calloc(1, sizeof(struct lthread))) == NULL) {
        perror("Failed to allocate memory for new lthread");
        return (errno);
    }

    if (posix_memalign(&lt->stack, getpagesize(), sched->stack_size)) {
        free(lt);
        perror("Failed to allocate stack for new lthread");
        return (errno);
    }

    lt->sched = sched;
    lt->stack_size = sched->stack_size;
    lt->state = BIT(LT_ST_NEW);
    lt->id = sched->spawned_lthreads++;
    lt->fun = fun;
    lt->fd_wait = -1;
    lt->arg = arg;
    lt->birth = _lthread_usec_now();
    *new_lt = lt;
    TAILQ_INSERT_TAIL(&lt->sched->ready, lt, ready_next);

    return (0);
}

void
lthread_set_data(void *data)
{
    lthread_get_sched()->current_lthread->data = data;
}

void *
lthread_get_data(void)
{
    return (lthread_get_sched()->current_lthread->data);
}

struct lthread*
lthread_current(void)
{
    return (lthread_get_sched()->current_lthread);
}

void
lthread_cancel(struct lthread *lt)
{
    if (lt == NULL)
        return;

    lt->state |= BIT(LT_ST_CANCELLED);
    _lthread_desched_sleep(lt);
    _lthread_cancel_event(lt);
    /*
     * we don't schedule the cancelled lthread if it was running in a compute
     * scheduler or pending to run in a compute scheduler or in an io worker.
     * otherwise it could get freed while it's still running.
     * when it's done in compute_scheduler, or io_worker - the scheduler will
     * attempt to run it and realize it's cancelled and abort the resumption.
     */
    if (lt->state & BIT(LT_ST_PENDING_RUNCOMPUTE) ||
        lt->state & BIT(LT_ST_WAIT_IO_READ) ||
        lt->state & BIT(LT_ST_WAIT_IO_WRITE) ||
        lt->state & BIT(LT_ST_RUNCOMPUTE))
        return;
    TAILQ_INSERT_TAIL(&lt->sched->ready, lt, ready_next);
}

int
lthread_cond_create(struct lthread_cond **c)
{
    if ((*c = calloc(1, sizeof(struct lthread_cond))) == NULL)
        return (-1);

    TAILQ_INIT(&(*c)->blocked_lthreads);

    return (0);
}

int
lthread_cond_wait(struct lthread_cond *c, uint64_t timeout)
{
    struct lthread *lt = lthread_get_sched()->current_lthread;
    TAILQ_INSERT_TAIL(&c->blocked_lthreads, lt, cond_next);

    _lthread_sched_busy_sleep(lt, timeout);

    if (lt->state & BIT(LT_ST_EXPIRED)) {
        TAILQ_REMOVE(&c->blocked_lthreads, lt, cond_next);
        return (-2);
    }

    return (0);
}

void
lthread_cond_signal(struct lthread_cond *c)
{
    struct lthread *lt = TAILQ_FIRST(&c->blocked_lthreads);
    if (lt == NULL)
        return;
    TAILQ_REMOVE(&c->blocked_lthreads, lt, cond_next);
    _lthread_desched_sleep(lt);
    TAILQ_INSERT_TAIL(&lthread_get_sched()->ready, lt, ready_next);
}

void
lthread_cond_broadcast(struct lthread_cond *c)
{
    struct lthread *lt = NULL;
    struct lthread *lttmp = NULL;

    TAILQ_FOREACH_SAFE(lt, &c->blocked_lthreads, cond_next, lttmp) {
        TAILQ_REMOVE(&c->blocked_lthreads, lt, cond_next);
        _lthread_desched_sleep(lt);
        TAILQ_INSERT_TAIL(&lthread_get_sched()->ready, lt, ready_next);
    }
}

void
lthread_sleep(uint64_t msecs)
{
    struct lthread *lt = lthread_get_sched()->current_lthread;

    if (msecs == 0) {
        TAILQ_INSERT_TAIL(&lt->sched->ready, lt, ready_next);
        _lthread_yield(lt);
    } else {
        _lthread_sched_sleep(lt, msecs);
    }
}

void
_lthread_renice(struct lthread *lt)
{
    lt->ops++;
    if (lt->ops < 5)
        return;

    TAILQ_INSERT_TAIL(&lthread_get_sched()->ready, lt, ready_next);
    _lthread_yield(lt);
}

void
lthread_wakeup(struct lthread *lt)
{
    if (lt->state & BIT(LT_ST_SLEEPING)) {
        TAILQ_INSERT_TAIL(&lt->sched->ready, lt, ready_next);
        _lthread_desched_sleep(lt);
    }
}

void
lthread_exit(void *ptr)
{
    struct lthread *lt = lthread_get_sched()->current_lthread;
    if (lt->lt_join && lt->lt_join->lt_exit_ptr && ptr)
        *(lt->lt_join->lt_exit_ptr) = ptr;

    lt->state |= BIT(LT_ST_EXITED);
    _lthread_yield(lt);
}

int
lthread_join(struct lthread *lt, void **ptr, uint64_t timeout)
{
    struct lthread *current = lthread_get_sched()->current_lthread;
    lt->lt_join = current;
    current->lt_exit_ptr = ptr;
    int ret = 0;

    /* fail if the lthread has exited already */
    if (lt->state & BIT(LT_ST_EXITED))
        return (-1);

    _lthread_sched_busy_sleep(current, timeout);

    if (current->state & BIT(LT_ST_EXPIRED)) {
        lt->lt_join = NULL;
        return (-2);
    }

    if (lt->state & BIT(LT_ST_CANCELLED))
        ret = -1;

    _lthread_free(lt);

    return (ret);
}

void
lthread_detach(void)
{
    struct lthread *current = lthread_get_sched()->current_lthread;
    current->state |= BIT(LT_ST_DETACH);
}

void
lthread_detach2(struct lthread *lt)
{
    lt->state |= BIT(LT_ST_DETACH);
}


void
lthread_set_funcname(const char *f)
{
    struct lthread *lt = lthread_get_sched()->current_lthread;
    strncpy(lt->funcname, f, 64);
}

uint64_t
lthread_id(void)
{
    return (lthread_get_sched()->current_lthread->id);
}

struct lthread*
lthread_self(void)
{
    return (lthread_get_sched()->current_lthread);
}

/*
 * convenience function for performance measurement.
 */
void
lthread_print_timestamp(char *msg)
{
	struct timeval t1 = {0, 0};
    gettimeofday(&t1, NULL);
	printf("lt timestamp: sec: %ld usec: %ld (%s)\n", t1.tv_sec, (long) t1.tv_usec, msg);
}
