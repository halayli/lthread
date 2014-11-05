Introduction
============

lthread is a multicore/multithread coroutine library written in C. It uses [Sam Rushing's](https://github.com/samrushing) _swap function to swap lthreads. What's special about lthread is that it allows you to make *blocking calls* and *expensive* computations, blocking IO inside a coroutine, providing you with the advantages of coroutines and pthreads. See the http server example below.

lthreads are created in userspace and don't require kernel intervention, they are light weight and ideal for socket programming. Each lthread have separate stack, and  the stack is madvise(2)-ed to save space, allowing you to create thousands(tested with a million lthreads) of coroutines and maintain a low memory footprint. The scheduler is hidden from the user and is created automagically in each pthread, allowing the user to take advantage of cpu cores and distribute the load by creating several pthreads, each running it's own lthread scheduler and handling its own share of coroutines. Locks are necessary when accessing global variables from lthreads running in different pthreads, and lthreads must not block on pthread condition variables as this will block the whole lthread scheduler in the pthread.

.. image:: images/lthread_scheduler.png

To run an lthread scheduler in each pthread, launch a pthread and create lthreads using lthread_create() followed by lthread_run() in each pthread.

**Scheduler**

The scheduler is build around epoll/kqueue and uses an rbtree to track which lthreads needs to run next.

If you need to execute an expensive computation or make a blocking call inside an lthread, you can surround the block of code with `lthread_compute_begin()` and `lthread_compute_end()`, which moves the lthread into an lthread_compute_scheduler that runs in its own pthread to avoid blocking other lthreads. lthread_compute_schedulers are created when needed and they die after 60 seconds of inactivity. `lthread_compute_begin()` tries to pick an already created and free lthread_compute_scheduler before it creates a new one.

Installation
------------

Currently, lthread is supported on FreeBSD, OS X, and Linux (x86 & 64bit arch).

To build and install, simply:

.. code-block:: Shell

    cmake .
    sudo make install

::

Linking
-------

`#include <lthread.h>`

Pass `-llthread` to gcc to use lthread in your program.

