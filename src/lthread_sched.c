/*
  lthread
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

  lthread_sched.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <inttypes.h>

#include "lthread_int.h"
#include "time_utils.h"
#include "rbtree.h"

static uint64_t _min_timeout(sched_t *);

static int  _lthread_poll(void);
static void _resume_expired_lthreads(sched_t *sched);

extern int errno;

static sched_node_t*
_rb_search(struct rb_root *root, uint64_t usecs)
{
    struct rb_node *node = root->rb_node;
    sched_node_t *data = NULL;

    while (node) {
        data = container_of(node, sched_node_t, node);

        if (usecs < data->usecs)
            node = node->rb_left;
        else if (usecs > data->usecs)
            node = node->rb_right;
        else {
            return data;
        }
    }

    return NULL;
}

static int
_rb_insert(struct rb_root *root, sched_node_t *data)
{
    sched_node_t *this = NULL;
    struct rb_node **new = &(root->rb_node), *parent = NULL;

    while (*new)
    {
        parent = *new;
        this = container_of(parent, sched_node_t, node);

        if (this->usecs > data->usecs)
            new = &((*new)->rb_left);
        else if (this->usecs < data->usecs)
            new = &((*new)->rb_right);
        else {
            assert(0);
            return -1;
        }
    }

    rb_link_node(&data->node, parent, new);
    rb_insert_color(&data->node, root);

    return 0;
}

static char tmp[100];
void
lthread_run(void)
{
    sched_t *sched;
    lthread_t *lt = NULL, *lttmp = NULL;
    int p = 0;
    int fd = 0;
    int ret = 0;

    sched = lthread_get_sched();
    /* scheduler not initiliazed, and no lthreads where created*/
    if (sched == NULL)
        return;

    while (sched->sleeping_state ||
        !LIST_EMPTY(&sched->new) ||
        sched->waiting_state) {

        /* 1. start by checking if a sleeping thread needs to wakeup */
        _resume_expired_lthreads(sched);

        /* 2. check to see if we have any new threads to run */
        while (!LIST_EMPTY(&sched->new)) {
            LIST_FOREACH_SAFE(lt, &sched->new, new_next, lttmp) {
                LIST_REMOVE(lt, new_next);
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
            pthread_mutex_unlock(&sched->compute_mutex);
            sched->sleeping_state--;
            _lthread_resume(lt);
        }

        /* 4. check if we received any events after lthread_poll */
        register_rd_interest(sched->compute_pipes[0]);
        _lthread_poll();

        /* 5. fire up lthreads that are ready to run */
        while (sched->total_new_events) {
            p = --sched->total_new_events;

            /* We got signaled via pipe to wakeup from polling & rusume compute.
             * Those lthreads will get handled in step 3.
             */ 
            fd = get_fd(&sched->eventlist[p]);
            if (fd == sched->compute_pipes[0]) {
                ret = read(fd, &tmp, sizeof(tmp));
                continue;
            }

            lt = (lthread_t *)get_data(&sched->eventlist[p]);
            if (lt == NULL)
                assert(0);

            if (is_eof(&sched->eventlist[p]))
                lt->state |= bit(LT_FDEOF);

            _desched_lthread(lt);
            _lthread_resume(lt);
        }
    }

    _sched_free(sched);

    return;
}

void
_lthread_wait_for(lthread_t *lt, int fd, lt_event_t e)
{

    
    //printf("registering lt %llu for %d\n", lt->id, e);
    if (lt->state & bit(LT_WAIT_READ) ||
        lt->state & bit(LT_WAIT_WRITE)) {
        printf("Unexpected event. lt id %"PRIu64" fd %d is already in %d state\n",
            lt->id, lt->fd_wait, lt->state);
        assert(0);
    }

    if (e == LT_READ)
        register_rd_interest(fd);
    else if (e == LT_WRITE)
        register_wr_interest(fd);
    else
        assert(0);

    lthread_get_sched()->waiting_state++;
    lt->fd_wait = fd;

    _lthread_yield(lt);
    clear_rd_wr_state(lt);
}

void
clear_rd_wr_state(lthread_t *lt)
{
    if (lt->fd_wait > 0) {
        //printf("%llu state is %d\n", lt->id, lt->state);
        if (lt->state & bit(LT_WAIT_READ)) {
            lt->state &= clearbit(LT_WAIT_READ);
        } else if (lt->state & bit(LT_WAIT_WRITE)) {
            lt->state &= clearbit(LT_WAIT_WRITE);
        } else {
            printf("lt->state is %d\n", lt->state);
            assert(0);
        }

        lt->sched->waiting_state--;
        lt->fd_wait = -1;
    }
}

static int
_lthread_poll(void)
{
    sched_t *sched;
    struct timespec t = {0, 0};
    int ret = 0;
    uint64_t usecs = 0;

    sched = lthread_get_sched();

    sched->total_new_events = 0;
    usecs = _min_timeout(sched);

    /* never sleep if we have an lthread pending in the new queue */
    if (usecs && LIST_EMPTY(&sched->new)) {
        t.tv_sec =  usecs / 1000000u;
        if (t.tv_sec != 0)
            t.tv_nsec  =  (usecs % 1000u)  * 1000000u;
        else
            t.tv_nsec = usecs * 1000u;
    }

    if (sched->nevents > MAX_CHANGELIST) {
        printf("too many events, %d\n", sched->nevents);
        assert(0);
    }

    ret = poll_events(t);

    if (ret == -1) {
        perror("error adding events to kevent");
        assert(0);
    }

    sched->nevents = 0;
    sched->total_new_events = ret;

    return 0;
}

void
_desched_lthread(lthread_t *lt)
{
    if (lt->state & bit(LT_SLEEPING)) {
        LIST_REMOVE(lt, sleep_next);
        lt->sched->sleeping_state--;
        /* del if lt is the last sleeping thread in the node */
        if (LIST_EMPTY(lt->sleep_list)) {
            rb_erase(&lt->sched_node->node, &lt->sched->sleeping);
            free(lt->sched_node);
            lt->sched_node = NULL;
        }
        lt->state &= clearbit(LT_SLEEPING);
        lt->state |= bit(LT_READY);
        lt->state &= clearbit(LT_EXPIRED);
    }
}

int
_sched_lthread(lthread_t *lt,  uint64_t msecs)
{
    sched_node_t *tmp = NULL;
    uint64_t t_diff_usecs = 0;
    int ret = 0;
    uint64_t usecs = msecs * 1000u;
    t_diff_usecs = tick_diff_usecs(lt->sched->birth, rdtsc()) + usecs;

    tmp = _rb_search(&lt->sched->sleeping, t_diff_usecs);
    if (tmp == NULL &&
        (tmp = calloc(1, sizeof(sched_node_t))) != NULL) {
        tmp->usecs = t_diff_usecs;
        ret = _rb_insert(&lt->sched->sleeping, tmp);
        if (ret == -1) {
            printf("Failed to insert node in rbtree!\n");
            assert(0);
            free(tmp);
            return -1;
        }
        LIST_INIT(&tmp->lthreads);
        LIST_INSERT_HEAD(&tmp->lthreads, lt, sleep_next);
        lt->sleep_list = &tmp->lthreads;
        lt->sched_node = tmp;
        lt->sched->sleeping_state++;
        lt->state |= bit(LT_SLEEPING);
        return 0;
    }
    if (tmp) {
        LIST_INSERT_HEAD(&tmp->lthreads, lt, sleep_next);
        lt->sched_node = tmp;
        lt->sleep_list = &tmp->lthreads;
        lt->sched->sleeping_state++;
        lt->state |= bit(LT_SLEEPING);
        return 0;
    } else {
        printf("impossible!\n");
        assert(0);
        return -1;
    }
}

static uint64_t
_min_timeout(sched_t *sched)
{
    struct rb_node *node = sched->sleeping.rb_node;
    sched_node_t *data = NULL;
    uint64_t t_diff_usecs = 0, min = 0;
    int in = 0;

    t_diff_usecs = tick_diff_usecs(sched->birth, rdtsc());
    min = sched->default_timeout;

    while (node) {
        data = container_of(node, sched_node_t, node);
        min = data->usecs;
        node = node->rb_left;
        in = 1;
    }

    if (!in)
        return min; 

    if (min > t_diff_usecs)
        return (min - t_diff_usecs);
    else /* we are running late on a thread, execute immediately */
        return 0;

    return 0;
}

static void
_resume_expired_lthreads(sched_t *sched)
{
    lthread_t *lt = NULL, *lttmp = NULL;
    struct rb_node *node = sched->sleeping.rb_node;
    sched_node_t *data = NULL;
    uint64_t t_diff_usecs = 0;
    _sched_node_l_t sched_list;
    LIST_INIT(&sched_list);

    /* current scheduler time */
    t_diff_usecs = tick_diff_usecs(sched->birth, rdtsc());

    while (node) {
        data = container_of(node, sched_node_t, node);
        node = node->rb_left;

        if (data->usecs <= t_diff_usecs) {
            LIST_INSERT_HEAD(&sched_list, data, next);
            LIST_FOREACH_SAFE(lt, &data->lthreads, sleep_next, lttmp) {
                LIST_REMOVE(lt, sleep_next);
                lt->sched->sleeping_state--;
                lt->state |= bit(LT_EXPIRED);
                lt->sleep_list = NULL;
                lt->sched_node = NULL;
                lt->state &= clearbit(LT_SLEEPING);
                if (lt->fd_wait > 0)
                    clear_interest(lt->fd_wait);

                if (_lthread_resume(lt) != -1)
                    lt->state &= clearbit(LT_EXPIRED);
            }
        } 
    }

    data = NULL;
    while (!LIST_EMPTY(&sched_list)) {
        data = LIST_FIRST(&sched_list);
        rb_erase(&data->node, &sched->sleeping);
        LIST_REMOVE(data, next);
        free(data);
    }

}
