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
pthread_key_t compute_sched_key;
pthread_once_t key_once = PTHREAD_ONCE_INIT;

LIST_HEAD(compute_sched_l, _compute_sched) compute_scheds = \
    LIST_HEAD_INITIALIZER(compute_scheds);
pthread_mutex_t sched_mutex = PTHREAD_MUTEX_INITIALIZER;

static void* _lthread_compute_run(void *arg);
static void _lthread_compute_resume(lthread_t *lt);
static compute_sched_t * _lthread_compute_sched_create(void);
static void _lthread_compute_sched_free(compute_sched_t *compute_sched);

int
lthread_compute_begin(void)
{
    sched_t *sched = lthread_get_sched();
    compute_sched_t *compute_sched = NULL, *tmp = NULL;
    lthread_t *lt = sched->current_lthread;
    void **stack = NULL;
    void **org_stack = NULL;

    /* search for an empty compute_scheduler */
    pthread_mutex_lock(&sched_mutex);
    LIST_FOREACH(tmp, &compute_scheds, compute_next) {
        if (tmp->state == COMPUTE_FREE) {
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
                pthread_mutex_unlock(&sched_mutex);
                return -1;
            }
        } else {
            LIST_INSERT_HEAD(&compute_scheds, compute_sched, compute_next);
        }
    }

    lt->compute_sched = compute_sched;

    lt->state |= bit(LT_PENDING_RUNCOMPUTE);
    pthread_mutex_lock(&lt->compute_sched->lthreads_mutex);
    LIST_INSERT_HEAD(&lt->compute_sched->lthreads, lt, compute_next);
    pthread_mutex_unlock(&lt->compute_sched->lthreads_mutex);

    pthread_mutex_unlock(&sched_mutex);

    /* yield function in scheduler to allow other lthreads to run while
     * this lthread runs in a pthread for expensive computations.
     */
    stack = (void **)(lt->compute_sched->stack + MAX_STACK_SIZE);
    org_stack = (void **)(lt->sched->stack + lt->sched->stack_size);

    _switch(&lt->sched->st, &lt->st);

    /* lthread_compute_begin() was called on the old stack. It pushed
     * esp to ebp when esp when pointing somewhere in the previous stack.
     * We need to change ebp to make it relative to the new stack address.
     * and we'll restore it back on lthread_compute_end.
     */
#ifdef __i386__
    asm("movl 0(%%ebp),%0" : "=r" (lt->ebp) :);
    asm("movl %0, 0(%%ebp)\n" ::"a"((void *)((intptr_t)stack - \
        ((intptr_t)org_stack - (intptr_t)lt->ebp))));
#elif defined(__x86_64__)
    asm("movq 0(%%rbp),%0" : "=r" (lt->ebp) :);
    asm("movq %0, 0(%%rbp)\n" ::"a"((void *)((intptr_t)stack - \
        ((intptr_t)org_stack - (intptr_t)lt->ebp))));
#endif

    return 0;
}

void
lthread_compute_end(void)
{
    /* get current compute scheduler */
    compute_sched_t *compute_sched =  pthread_getspecific(compute_sched_key);
    lthread_t *lt = compute_sched->current_lthread;
    lt->state &= clearbit(LT_RUNCOMPUTE);
    _switch(&compute_sched->st, &lt->st);
    /* restore ebp back to its relative old stack address */
#ifdef __i386__
    asm("movl %0, 0(%%ebp)\n" ::"a"(lt->ebp));
#elif defined(__x86_64__)
    asm("movq %0, 0(%%rbp)\n" ::"a"(lt->ebp));
#endif
}

void
_lthread_compute_add(lthread_t *lt)
{

    void **stack = NULL;
    void **org_stack = NULL;

    stack = (void **)(lt->compute_sched->stack + MAX_STACK_SIZE);
    org_stack = (void **)(lt->sched->stack + lt->sched->stack_size);

    /* change ebp esp to be relative to the new stack address */
    lt->st.ebp = lt->compute_sched->st.ebp = (void*)((intptr_t)stack - \
         ((intptr_t)org_stack - (intptr_t)(lt->st.ebp)));
    lt->st.esp = lt->compute_sched->st.esp = (void*)((intptr_t)stack - \
        lt->stack_size);

    pthread_mutex_lock(&lt->compute_sched->lthreads_mutex);
    lt->state &= clearbit(LT_PENDING_RUNCOMPUTE);
    lt->state |= bit(LT_RUNCOMPUTE);
    pthread_mutex_unlock(&lt->compute_sched->lthreads_mutex);
    /* wakeup pthread if it was sleeping */
    pthread_mutex_lock(&lt->compute_sched->run_mutex);
    pthread_cond_signal(&lt->compute_sched->run_mutex_cond);
    pthread_mutex_unlock(&lt->compute_sched->run_mutex);

}

static void
_lthread_compute_sched_free(compute_sched_t *compute_sched)
{
    pthread_mutex_destroy(&compute_sched->run_mutex);
    pthread_mutex_destroy(&compute_sched->lthreads_mutex);
    pthread_cond_destroy(&compute_sched->run_mutex_cond);
    free(compute_sched);
}

static compute_sched_t *
_lthread_compute_sched_create(void)
{
    compute_sched_t *compute_sched = NULL;
    pthread_t pthread;

    if ((compute_sched = calloc(1, sizeof(compute_sched_t))) == NULL)
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
    pthread_detach(pthread);

    LIST_INIT(&compute_sched->lthreads);

    return compute_sched;
}


static int
_lthread_compute_save_exec_state(lthread_t *lt)
{
    void *stack_top = NULL;
    void **stack = NULL;
    void **org_stack = NULL;
    size_t size = 0;

    stack_top = lt->compute_sched->stack + MAX_STACK_SIZE;
    size = stack_top - lt->st.esp;

    if (size && lt->stack_size != size) {
        if (lt->stack) {
            free(lt->stack);
        }
        if ((lt->stack = calloc(1, size)) == NULL) {
            perror("Failed to allocate memory to save stack\n");
            abort();
            return errno;
        }
    }

    lt->stack_size = size;
    if (size)
        memcpy(lt->stack, lt->st.esp, size);

    stack = (void **)(lt->compute_sched->stack + MAX_STACK_SIZE);
    org_stack = (void **)(lt->sched->stack + lt->sched->stack_size);

    /* change ebp & esp back to be relative to the old stack address */
    lt->st.ebp = (void*)((intptr_t)org_stack - ((intptr_t)stack - \
        (intptr_t)(lt->st.ebp)));
    lt->st.esp = (void*)((intptr_t)org_stack - lt->stack_size);

    return 0;
}

static void
_lthread_compute_resume(lthread_t *lt)
{
    _restore_exec_state(lt);
    _switch(&lt->st, &lt->compute_sched->st);
    _lthread_compute_save_exec_state(lt);
}

void
once_routine(void)
{
    if (pthread_key_create(&compute_sched_key, NULL)) {
        perror("Failed to allocate compute sched key");
        assert(0);
    }
}

static void*
_lthread_compute_run(void *arg)
{
    compute_sched_t *compute_sched = arg;
    lthread_t *lt = NULL;
    struct timespec timeout;
    int status = 0;

    pthread_once(&key_once, once_routine);

    pthread_setspecific(compute_sched_key, arg);

    while (1) {

        /* resume lthreads to run their computation or make a blocking call */
        while (1) {
            pthread_mutex_lock(&compute_sched->lthreads_mutex);

            /* we have no work to do, break and wait 60 secs then exit */
            if (LIST_EMPTY(&compute_sched->lthreads)) {
                pthread_mutex_unlock(&compute_sched->lthreads_mutex);
                break;
            }

            lt = LIST_FIRST(&compute_sched->lthreads);
            if (lt->state & bit(LT_PENDING_RUNCOMPUTE)) {
                pthread_mutex_unlock(&compute_sched->lthreads_mutex);
                continue;
            }

            LIST_REMOVE(lt, compute_next);

            pthread_mutex_unlock(&compute_sched->lthreads_mutex);

            compute_sched->current_lthread = lt;
            compute_sched->state = COMPUTE_BUSY;

            _lthread_compute_resume(lt);

            compute_sched->current_lthread = NULL;
            compute_sched->state = COMPUTE_FREE;

            /* resume it back on the  prev scheduler */
            pthread_mutex_lock(&lt->sched->compute_mutex);
            LIST_INSERT_HEAD(&lt->sched->compute, lt, compute_sched_next);
            pthread_mutex_unlock(&lt->sched->compute_mutex);

            /* signal the prev scheduler in case it was sleeping in a poll */
            write(lt->sched->compute_pipes[1], "1", 1);
        }

        pthread_mutex_lock(&compute_sched->run_mutex);
        /* wait if we have no work to do, exit */
        timeout.tv_sec = time(NULL) + THREAD_TIMEOUT_BEFORE_EXIT; 
        timeout.tv_nsec = 0;
        status = pthread_cond_timedwait(&compute_sched->run_mutex_cond,
            &compute_sched->run_mutex, &timeout);
        pthread_mutex_unlock(&compute_sched->run_mutex);

        /* if we didn't timeout, then we got signaled to do some work */
        if (status != ETIMEDOUT)
            continue;

        /* lock the global sched to check if we have any pending work to do */
        pthread_mutex_lock(&sched_mutex);

        pthread_mutex_lock(&compute_sched->lthreads_mutex);
        if (LIST_EMPTY(&compute_sched->lthreads)) {

            LIST_REMOVE(compute_sched, compute_next);

            pthread_mutex_unlock(&compute_sched->lthreads_mutex);
            pthread_mutex_unlock(&sched_mutex);
            _lthread_compute_sched_free(compute_sched);
            break;
        }

        pthread_mutex_unlock(&compute_sched->lthreads_mutex);
        pthread_mutex_unlock(&sched_mutex);
    }


    return NULL;
}
