Lthread
=======

lthread_create
--------------
.. c:function:: lthread_create(lthread_t **new_lt, lthread_func func, void *arg)

    Creates a new lthread.

    :param lthread_t** new_lt: a ptr->ptr to store the new lthread structure on
                               success
    :param lthread_func func: Function to run in an lthread.
    :param void* arg: Argument to pass to `func` when called.
    :return: 0 on success with new_lt pointing to the new lthread,
             -1 on failure with `errno` specifying the reason.

    .. c:type:: lthread_func: void (*)(void*)

lthread_sleep
--------------
.. c:function:: lthread_sleep(uint64_t msecs)

    Causes an lthread to sleep for `msecs` milliseconds.

    :param uint64_t msecs: Number of milliseconds to sleep. `msecs=0` causes the
                           lthread to yield and allow other lthreads to resume
                           before it continues.


lthread_cancel
--------------
.. c:function:: void lthread_cancel(lthread_t *lt)

    Cancels lthread and prepares it to be removed from lthread scheduler. If it
    was waiting for events, the events will get cancelled. If an lthread was
    joining on it, the lthread joining will get scheduled to run.

    :param lthread_t* lt: lthread to cancel

lthread_run
-----------
.. c:function:: void lthread_run(void)

    Runs lthread scheduler until all lthreads return.

lthread_join
------------
.. c:function:: int lthread_join(lthread_t *lt, void **ptr, uint64_t timeout)

    Blocks the calling lthread until lt has exited or a timeout occurred. In
    case of timeout, lthread_join returns -2 and lt doesn't get freed. If target
    lthread was cancelled, it returns -1 and the target lthread will be freed.
    \*\*ptr will get populated by lthread_exit(). ptr cannot be from lthread's
    stack space. Joining on a joined lthread has undefined behavior.

    :param lthread_t* lt: lthread to join on.
    :param void** ptr: optional, this ptr will be populated by :c:func:`lthread_exit()`.
    :param uint64_t timeout: How long to wait trying to join on lt before timing out.

    return: 0 on success, -1 on cancelled target lthread, -2 on timeout.

    .. ATTENTION:: Joining on a joined lthread has undefined behavior

lthread_detach
--------------
.. c:function:: void lthread_detach(void)

    Marks the current lthread as detached, causing it to get freed once it exits.
    Otherwise :c:func:`lthread_join()` must be called on the lthread to free it
    up. If an lthread wasn't marked as detached and wasn't joined on then
    a memory leak occurs.

lthread_detach2
----------------
.. c:function:: void lthread_detach2(lthread_t *lt)

    Same as :c:func:`lthread_detach()` except that it doesn't have to be called
    from within the lthread function. The lthread to detach is passed as a param.

    :param lthread_t* lt: Lthread to detach.


lthread_exit
------------
.. c:function:: void lthread_exit(void *ptr)

    Sets ptr value for the lthread calling :c:func:`lthread_join()` and exits lthread.

    :param void* ptr: Optional, ptr value to pass to the joining lthread.


lthread_wakeup
--------------
.. c:function:: void lthread_wakeup(lthread_t *lt)

    Wakes up a sleeping lthread. If lthread wasn't sleeping this function has
    no effect.

    :param lthread_t* lt: The lthread to wake up.

lthread_cond_create
-------------------
.. c:function:: int lthread_cond_create(lthread_cond_t **c)

     Creates a condition variable that can be used between lthreads to block/signal each other.

     :param lthread_cond_t** c: ptr->ptr that will be populated on success.

     :return: 0 on success -1 on error with `errno` containing the reason.


lthread_cond_wait
-----------------
.. c:function:: int lthread_cond_wait(lthread_cond_t *c, uint64_t timeout)

    Puts the lthread calling :c:func:`lthread_cond_wait()` to sleep until
    `timeout` expires or another lthread signals it.

    :param lthread_cond_t* c: condition variable created by :c:func:`lthread_cond_create()`
                              and shared between lthreads requiring synchronization.
    :param uint64_t timeout: Number of milliseconds to wait on the condition
                             variable to be signaled before it times out. 0 to
                             wait indefinitely.

    :return: 0 if it was signal or -2 if it timed out waiting.

lthread_cond_signal
-------------------
.. c:function:: void lthread_cond_signal(lthread_cond_t *c)

    Signals a single lthread blocked on :c:func:`lthread_cond_wait()` to wake up and resume.

    :param lthread_cond_t* c: condition variable created by :c:func:`lthread_cond_create()`
                              and shared between lthreads requiring synchronization.


lthread_cond_broadcast
----------------------
.. c:function:: void lthread_cond_broadcast(lthread_cond_t *c)

    Signals all lthreads blocked on :c:func:`lthread_cond_wait()` to wake up and resume.

    :param lthread_cond_t* c: condition variable created by :c:func:`lthread_cond_create()`
                              and shared between lthreads requiring synchronization.

lthread_set_data
----------------
.. c:function:: void lthread_set_data(void *data)

    Sets data bound to the lthread. This value can be retrieved anywhere in
    the lthread using :c:func:`lthread_get_data()`.

    :param void* data: value to be set.


lthread_get_data
----------------
.. c:function:: void *lthread_get_data(void);

    Returns the value set for the current lthread.

    :return: Value set by :c:func:`lthread_set_data()`


lthread_current
---------------
.. c:function:: lthread_t *lthread_current();

    Returns a pointer to the current lthread.

    :return: ptr to the current lthread running.


lthread_compute_begin
---------------------
.. c:function:: int lthread_compute_begin(void)

    Resumes lthread inside a pthread to run expensive computations or make a
    blocking call like `gethostbyname()`. This call *must* be followed by
    :c:func:`lthread_compute_end()` after the computation and/or blocking calls
    have been made to resume the lthread in its original lthread scheduler.
    No lthread_* calls can be made during the 2 calls.

lthread_compute_end
-------------------
.. c:function:: void lthread_compute_end(void);

    Moves lthread from pthread back to the lthread scheduler it was running on.

DEFINE_LTHREAD
--------------

.. c:macro:: DEFINE_LTHREAD(name)

    Sets the name of the function inside the lthread structure for easier
    crash debugging. Must be called inside the lthread.
