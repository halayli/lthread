lthread
======

Introduction
------------

lthread is a coroutine library written in C. It uses [Sam Rushing's](https://github.com/samrushing) _swap function to swap lthreads.

Installation
------------

Currently, lthread is only supported on FreeBSD and Linux. OS X support will becoming soon.

`cd src`

`sudo make install`

Usage
-----

`#include <lthread.h>`

Pass `-llthread` to gcc to use lthread in your program.

Examples
--------

`gcc -I/usr/local/include -llthread test.c -o test`

### A simple timer in lthread

```C

#include <lthread.h>
#include <stdio.h>
#include <sys/time.h>

void
a(lthread_t *lt, void *x)
{
    int i = 3;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in a\n");

    while (i--) {
        gettimeofday(&t1, NULL);
        lthread_sleep(2000);
        gettimeofday(&t2, NULL);
        printf("a (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
    }
    printf("a is exiting\n");
}

void
b(lthread_t *lt, void *x)
{
    int i = 8;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in b\n");

    while (i--) {
        gettimeofday(&t1, NULL);
        lthread_sleep(1000);
        gettimeofday(&t2, NULL);
        printf("b (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
    }
    printf("b is exiting\n");
}

int
main(int argc, char **argv)
{
    lthread_t *lt = NULL;

    lthread_create(&lt, a, NULL);
    lthread_create(&lt, b, NULL);
    lthread_join();

    return 0;
}

```


Library calls
-------------

```int     lthread_create(lthread_t **new_lt, void *fun, void *arg);```

Creates a new lthread and returns it via `*new_lt`.

Returns 0 if success, -1 on failure.

```void    lthread_destroy(lthread_t *lt);```

Destroys an lthread and cancels any events it was expecting.

```void    lthread_join(void);```

Blocks until all lthreads created have exited.

```void    lthread_sleep(uint64_t msecs);```

Puts an lthread to sleep until msecs have passed.

```void    lthread_wakeup(lthread_t *lt);```

Wake up a sleeping lthread.

```int     lthread_cond_create(lthread_cond_t **c);```

Creates a condition variable that can be used between lthreads to block/signal each other.

```int     lthread_cond_wait(lthread_cond_t *c, uint64_t timeout);```

Puts the lthread calling `lthread_cond_wait` to sleep until `timeout` expires or another thread signals it.

Returns 0 if it was signaled or -2 if it expired.

```void    lthread_cond_signal(lthread_cond_t *c);```

Signals an lthread blocked on `lthread_cond_wait` to wake up and resume.

```void    *lthread_get_data(void);```

Returns the value set for the current lthread.

```void    lthread_set_data(void *data);```

Sets data bound to the lthread. This value can be retrieved anywhere in the lthread using `lthread_get_data`.

```uint64_t lthread_id();```

Returns the lthread Id. 

```lthread_t *lthread_current();```

Returns a pointer to the current lthread.

```void    lthread_set_funcname(const char *f);```

Sets the lthread method name to the current function. It makes debugging easier by knowing which function a specific lthread was executing.

```char    *lthread_summary();```

Returns a string describing all the executing lthreads and their state. This string must be freed using `free()`

###Socket related functions

Refer to the appropriate man pages of each function to learn about their arguments.

```int lthread_socket(int domain, int type, int protocol);```

An lthread version of socket(2).

```int lthread_accept(int fd, struct sockaddr *, socklen_t *);```

An lthread version of accept(2).

```int lthread_close(int fd);```

Close an lthread_socket.

```int lthread_connect(int fd, struct sockaddr *, socklen_t, uint32_t timeout);```

An lthread version of connect(2) with an additional argument `timeout` to specify how long the function waits before it gives up on connecting.

Returns 0 on a successful connection or -2 if it expired waiting.

```ssize_t lthread_recv(int fd, void * buf, size_t buf_len, int flags, unsigned int timeout);```

An lthread version of recv(2) with an additional argument `timeout` to specify how long to wait before it gives up on receiving.

Returns the number of bytes received or -2 if it expired waiting.

```ssize_t lthread_send(int fd, const void *buf, size_t buf_len, int flags);```

An lthread version of send(2).

```ssize_t lthread_writev(int fd, struct iovec *iov, int iovcnt);```

An lthread version of writev(2).

*freebsd only*

```int lthread_sendfile(int fd, int s, off_t offset, size_t nbytes, struct sf_hdtr *hdtr);```

An lthread version of FreeBSD sendfile(2).
