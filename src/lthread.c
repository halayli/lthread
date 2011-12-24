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

  lthread.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <inttypes.h>

#include "common/time.h"
#include "common/rbtree.h"
#include "lthread_int.h"
#include "poller.h"

extern int errno;

static void _exec(void *lt);
static int  _restore_exec_state(lthread_t *lt);
static int  _save_exec_state(lthread_t *lt);
static void _lthread_init(lthread_t *lt);

pthread_key_t lthread_sched_key;

int _switch(struct _cpu_state *new_state, struct _cpu_state *cur_state);
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
    ((lthread_t *)lt)->fun(lt, ((lthread_t *)lt)->arg);
    ((lthread_t *)lt)->state = bit(LT_EXITED);

    _lthread_yield(lt);
}

void
_lthread_yield(lthread_t *lt) 
{
    _switch(&lt->sched->st, &lt->st);
}

void
_lthread_free(lthread_t *lt)
{
    free(lt->stack);
    free(lt);
}

int
_lthread_resume(lthread_t *lt)
{
    if (lt->state & bit(LT_NEW))
        _lthread_init(lt);

    _restore_exec_state(lt);

    lthread_get_sched()->current_lthread = lt;
    _switch(&lt->st, &lt->sched->st);

    if (lt->state & bit(LT_EXITED)) {
        _lthread_free(lt);
        return -1;
    } else {
        return _save_exec_state(lt);
    }
}

static void
_lthread_key_destructor(void *data)
{
    free(data);
}

static void
_lthread_key_create()
{
    if (lthread_sched_key)
        return;

    if (pthread_key_create(&lthread_sched_key, _lthread_key_destructor)) {
        perror("Failed to allocate sched key\n");
        return;
    }
}

int
lthread_init(size_t size)
{
    _lthread_key_create();
    return sched_create(size);
}

static void
_lthread_init(lthread_t *lt)
{
    void **stack = NULL;
    stack = (void **)(lt->sched->stack + lt->sched->stack_size);

    stack[-3] = NULL;
    stack[-2] = (void *)lt;
    lt->st.esp = (void *)stack - (4 * sizeof(void *));
    lt->st.ebp = (void *)stack - (3 * sizeof(void *));
    lt->st.eip = (void *)_exec;
    lt->state = bit(LT_READY);
}

static int
_restore_exec_state(lthread_t *lt)
{
    if (lt->stack_size) {
        memcpy(lt->st.esp, lt->stack, lt->stack_size);
    }

    return 0;
}

static int
_save_exec_state(lthread_t *lt)
{
    void *stack_top = NULL;
    size_t size = 0;

    stack_top = lt->sched->stack + lt->sched->stack_size;
    size = stack_top - lt->st.esp;

    if (size && lt->stack_size != size) {
        if (lt->stack) {
            free(lt->stack);
        }
        if ((lt->stack = calloc(1, size)) == NULL) {
            perror("Failed to allocate memory to save stack\n");
            return errno;
        }
    }

    lt->stack_size = size;
    if (size)
        memcpy(lt->stack, lt->st.esp, size);

    return 0;
}

void
_sched_free(sched_t *sched)
{
    free(lthread_get_sched()->stack);
    free(lthread_get_sched());
    pthread_setspecific(lthread_sched_key, NULL);
}

int
sched_create(size_t stack_size)
{
    sched_t *new_sched;
    size_t sched_stack_size = 0;

    sched_stack_size = stack_size ? stack_size : MAX_STACK_SIZE;
    
    if ((new_sched = calloc(1, sizeof(sched_t))) == NULL) {
        perror("Failed to initialize scheduler\n");
        return errno;
    }

    pthread_setspecific(lthread_sched_key, new_sched);

    if ((new_sched->stack = calloc(1, sched_stack_size)) == NULL) {
        free(new_sched);
        perror("Failed to initialize scheduler\n");
        return errno;
    }

    bzero(new_sched->stack, sched_stack_size);

    if ((new_sched->poller = create_poller()) == -1) {
        perror("Failed to initialize poller\n");
        _sched_free(new_sched);
        return errno;
    }

    new_sched->stack_size = sched_stack_size;

    new_sched->total_lthreads = 0;
    new_sched->default_timeout = 3000000u;
    new_sched->waiting_state = 0;
    new_sched->sleeping_state = 0;
    new_sched->sleeping = RB_ROOT;
    new_sched->birth = rdtsc();
    LIST_INIT(&new_sched->new);

    bzero(&new_sched->st, sizeof(struct _cpu_state));

    return 0;
}

int
lthread_create(lthread_t **new_lt, void *fun, void *arg)
{
    lthread_t *lt = NULL;

    if ((lt = calloc(1, sizeof(lthread_t))) == NULL) {
        perror("Failed to allocate memory for new lthread\n");
        return errno;
    }

    _lthread_key_create();

    if (lthread_get_sched() == NULL)
        sched_create(0);

    lt->sched = lthread_get_sched();
    lt->stack = NULL;
    lt->stack_size = 0;
    lt->state = bit(LT_NEW);
    lt->id = lthread_get_sched()->total_lthreads++;
    lt->fun = fun;
    lt->fd_wait = -1;
    lt->arg = arg;
    lt->birth = rdtsc();
    lt->timeout = -1;
    lt->new_next.le_next = NULL;
    lt->new_next.le_next = NULL;
    lt->sleep_next.le_prev = NULL;
    lt->sleep_next.le_prev = NULL;
    lt->sleep_list = NULL;
    lt->sched_node = NULL;
    lt->data = NULL;
    *new_lt = lt;
    LIST_INSERT_HEAD(&lt->sched->new, lt, new_next);

    return 0;
}

void
lthread_set_data(void *data)
{
    lthread_get_sched()->current_lthread->data = data;
}

void *
lthread_get_data(void)
{
    return lthread_get_sched()->current_lthread->data;
}

lthread_t *
lthread_current(void)
{
    return lthread_get_sched()->current_lthread;
}

void
lthread_destroy(lthread_t *lt)
{
    if (lt == NULL)
        return;

    _desched_lthread(lt);
    if (lt->fd_wait > 0) { /* was it waiting on an event ? */
        clear_interest(lt->fd_wait);
    } else { /* it got to be in new queue */
        LIST_REMOVE(lt, new_next);
    }
}

int
lthread_cond_create(lthread_cond_t **c)
{
    if ((*c = calloc(1, sizeof(lthread_cond_t))) == NULL)
        return -1;
    (*c)->blocked_lthread = NULL;

    return 0;
}

int
lthread_cond_wait(lthread_cond_t *c, uint64_t timeout)
{
    lthread_t *lt = lthread_get_sched()->current_lthread;
    c->blocked_lthread = lt;
    if (timeout) {
        _sched_lthread(lt, timeout);
    }

    lt->state |= bit(LT_LOCKED);
    _lthread_yield(lt);
    lt->state &= clearbit(LT_LOCKED);

    if (lt->state & bit(LT_EXPIRED)) {
        return -2;
    } else {
        _desched_lthread(lt);
    }

    return 0;
}

void
lthread_cond_signal(lthread_cond_t *c)
{
    if (c->blocked_lthread != NULL) {
        LIST_INSERT_HEAD(&lthread_get_sched()->new, c->blocked_lthread, new_next);
    }

    c->blocked_lthread = NULL;
}

void
lthread_sleep(uint64_t msecs)
{
    lthread_t *lt = lthread_get_sched()->current_lthread;
    lt->timeout = msecs;
    lt->ticks = rdtsc();
    lt->fd_wait = -1;
    if (_sched_lthread(lt, lt->timeout) == -1) {
        lt->state = bit(LT_READY);
        return;
    }
    lt->state |= bit(LT_SLEEPING);
    _lthread_yield(lt);
}

void lthread_wakeup(lthread_t *lt)
{
    if (lt->state & bit(LT_SLEEPING)) {
        LIST_INSERT_HEAD(&lt->sched->new, lt, new_next);
        _desched_lthread(lt);
    }
}

int
lthread_running()
{
    return lthread_get_sched()->waiting_state;
}

int
lthread_sleeping()
{
    return lthread_get_sched()->sleeping_state;
}

void
lthread_set_funcname(const char *f)
{
    lthread_t *lt = lthread_get_sched()->current_lthread;
    strncpy(lt->funcname, f, 64);
}

uint64_t
lthread_id(void)
{
    return lthread_get_sched()->current_lthread->id;
}
