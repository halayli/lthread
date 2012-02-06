lthread
=======

Introduction
------------

lthread is a multicore/multithread coroutine library written in C. It uses [Sam Rushing's](https://github.com/samrushing) _swap function to swap lthreads. lthread allows you to make blocking calls and expensive computations inside a coroutine as long as you surround your code with lthread_compute_begin()/lthread_compute_end(), hence combining the advantages of coroutines and pthreads. See the http server example below. 

lthreads run inside an lthread scheduler. The scheduler is hidden from the user and is created automagically in each pthread, allowing the user to take advantage of cpu cores and distribute the load. Locks are necessary when accessing global variables from lthreads running in different pthreads, and lthreads must not block on condition variables as this will block the whole scheduler in the pthread.

![](https://github.com/halayli/lthread/blob/master/images/lthread_scheduler.png?raw=true "Lthread scheduler")

To run an lthread scheduler in each pthread, launch the pthread and create the lthreads using lthread_create() followed by lthread_run() in each pthread.


### lthread scheduler's inner works

The lthread scheduler has a main stack that it uses to execute/resume lthreads on, and before it yields, the scheduler saves the current stack state + registers into the lthread and copies it back on the scheduler stack when it resumes.

The scheduler is build around epoll/kqueue and uses an rbtree to track which lthreads needs to run next.

If you need to execute an expensive computation or make a blocking call inside an lthread, you can surround the block of code with `lthread_compute_begin()` and `lthread_compute_end()`, which moves the lthread into an lthread_compute_scheduler that runs in a pthread to avoid blocking other lthreads. lthread_compute_schedulers are created when needed and they die after 60 seconds of inactivity. `lthread_compute_begin()` tries to pick an already created and free lthread_compute_scheduler before it creates a new one.

Installation
------------

Currently, lthread is supported on FreeBSD, OS X,  and Linux.

`cd src`

`sudo make install`

Usage
-----

`#include <lthread.h>`

Pass `-llthread` to gcc to use lthread in your program.

Library calls
-------------

```C
/*
 * Creates a new lthread and returns it via `*new_lt`.
 * Returns 0 if success, -1 on failure.
 */
int lthread_create(lthread_t **new_lt, void *fun, void *arg);
```

```C
/*
 * Destroys an lthread and cancels any events it was expecting.
 */
void lthread_destroy(lthread_t *lt);
```

```C
/*
 * Blocks until all lthreads created have exited.
 */
void lthread_run(void);
```

```C
/*
 * Marks the current lthread as detached, causing it to
 * get freed once it exits. Otherwise `lthread_join()` must be
 * called on the lthread to free it up.
 * If an lthread wasn't marked as detached and wasn't joined on then
 * a memory leak occurs.
 */
void lthread_detach(void);
```

```C
/*
 * Blocks the calling thread until lt has exited or timeout occured.
 * In case of timeout, lthread_join returns -2 and lt doesn't get freed.
 * If you don't want to join again on the lt, make sure to call lthread_destroy()
 * to free up the the lthread else a leak occurs.
 * **ptr will get populated by lthread_exit(). ptr cannot be from lthread's
 * stack space.
 * Returns 0 on success or -2 on timeout.
 */
int lthread_join(lthread_t *lt, void **ptr, uint_64 timeout);
```

```C
/*
 * Sets ptr value for the joining lthread and exits lthread.
 */
void lthread_exit(void **ptr);
```

```C
/*
 * Moves lthread into a pthread to run its expensive computation or make a blocking 
 * call like `gethostbyname()`.
 * This call *must* be followed by lthread_compute_end() after the computation and/or
 * blocking calls have been made to resume the lthread in its original lthread scheduler.
 * No lthread_* calls can be made during lthread_compute_begin()/lthread_compute_end(). 
 */
 void lthread_compute_begin(void);
```

```C
/*
 * Moves lthread from pthread back to the lthread scheduler it was running on.
 */
 void lthread_compute_end(void);
```

```C
/*
 * Puts an lthread to sleep until msecs have passed.
 */
void lthread_sleep(uint64_t msecs);
```

```C
/* 
 * Wake up a sleeping lthread. 
 */
void lthread_wakeup(lthread_t *lt);
```

```C
/* 
 * Creates a condition variable that can be used between lthreads to block/signal each other.
 */
int lthread_cond_create(lthread_cond_t **c);
```

```C
/*
 * Puts the lthread calling `lthread_cond_wait` to sleep until `timeout` expires or another thread signals it.
 * Returns 0 if it was signaled or -2 if it expired.
 */
int lthread_cond_wait(lthread_cond_t *c, uint64_t timeout);
```


```C
/*
 * Signals an lthread blocked on `lthread_cond_wait` to wake up and resume.
 */
void lthread_cond_signal(lthread_cond_t *c);
```


```C
/*
 * Returns the value set for the current lthread.
 */
void *lthread_get_data(void);
```

```C
/* 
 * Sets data bound to the lthread. This value can be retrieved anywhere in the lthread using `lthread_get_data()`.
 */
void lthread_set_data(void *data);
```


```C
/*
 * Returns the lthread Id.
 */
uint64_t lthread_id();
```


```C
/*
 * Returns a pointer to the current lthread.
 */
lthread_t *lthread_current();
```

```C
/*
 * Sets the lthread method name to the current function.
 * It makes debugging easier by knowing which function a specific lthread was executing.
 */
void lthread_set_funcname(const char *f);
```

###Socket related functions

Refer to the appropriate man pages of each function to learn about their arguments.

```C
/*
 * An lthread version of socket(2).
 * Returns a socket fd.
 */
int lthread_socket(int domain, int type, int protocol);
```


```C
/*
 * An lthread version of accept(2).
 */
int lthread_accept(int fd, struct sockaddr *, socklen_t *);
```


```C
/*
 * Close an lthread_socket.
 */
int lthread_close(int fd);
```


```C
/*
 * An lthread version of connect(2) with an additional argument `timeout` to specify how 
 * long the function waits before it gives up on connecting.
 * `timeout` of 0 means no timeout.
 * Returns 0 on a successful connection or -2 if it expired waiting.
 */
int lthread_connect(int fd, struct sockaddr *, socklen_t, uint64_t timeout);
```


```C
/*
 * An lthread version of recv(2) with an additional argument `timeout` to 
 * specify how long to wait before it gives up on receiving.
 * `timeout` of 0 means no timeout.
 * Returns the number of bytes received or -2 if it expired waiting.
 */
ssize_t lthread_recv(int fd, void * buf, size_t buf_len, int flags, uint64_t timeout);
```

```C
/*
 * An lthread version of recvfrom(2) with an additional argument `timeout` to 
 * specify how long to wait before it gives up on receiving.
 * `timeout` of 0 means no timeout.
 * Returns the number of bytes received or -2 if it expired waiting.
 */
ssize_t lthread_recvfrom(int fd, void *restrict buffer, size_t length, int flags,
    struct sockaddr *restrict address, socklen_t *restrict address_len, uint64_t timeout);
```

```C
/*
 * An lthread version of recvmsg(2) with an additional argument `timeout` to 
 * specify how long to wait before it gives up on receiving.
 * `timeout` of 0 means no timeout.
 * Returns the number of bytes received or -2 if it expired waiting.
 */
ssize_t lthread_recvmsg(int socket, struct msghdr *message, int flags, uint64_t timeout);
```

```C
/*
 * An lthread version of send(2).
 */
ssize_t lthread_send(int fd, const void *buf, size_t buf_len, int flags);
```

```C
/*
 * An lthread version of sendmsg(2).
 */
ssize_t lthread_sendmsg(int fd, const void *buf, size_t buf_len, int flags);
```

```C
/*
 * An lthread version of sendto(2).
 */
ssize_t lthread_sendto(int socket, const void *buffer, size_t length, int flags,
    const struct sockaddr *dest_addr, socklen_t dest_len);
```

```C
/*
 * An lthread version of writev(2).
 */
ssize_t lthread_writev(int fd, struct iovec *iov, int iovcnt);
```

*freebsd only*

```C
/*
 * An lthread version of FreeBSD sendfile(2).
 */
int lthread_sendfile(int fd, int s, off_t offset, size_t nbytes, struct sf_hdtr *hdtr);
```

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
    lthread_detach();

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
    lthread_detach();

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
    lthread_run();

    return 0;
}

```

### An incomplete web server to illustrate some of lthread features

In this example, the dummy http server will accept a connection, runs a relatively expensive fibonacci(35) in lthread_compute_begin() / lthred_compute_end() and replies back.

While fibonacci is running, we'll continue to accept new connections and handle them without blocking because fibonacci is running between lthread_compute_begin() and lthread_compute_end(), which moves the lthread into a pthread and resumes it there until lthread_compute_end() is called and that's when it moves back to its previous scheduler.


`gcc -I/usr/local/include -llthread test.c -o test`

```C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lthread.h>

struct cli_info {
    /* other stuff if needed*/
    struct sockaddr_in peer_addr;
    int fd;
};

typedef struct cli_info cli_info_t;

char *reply = "HTTP/1.0 200 OK\r\nContent-length: 11\r\n\r\nHello World";

unsigned int
fibonacci(unsigned int n)
{
    if (n == 0)
        return 0;
     if (n == 1)
        return 1;

     return fibonacci(n - 1) + fibonacci(n - 2);
}

void
http_serv(lthread_t *lt, void *arg)
{
    cli_info_t *cli_info = arg;
    char *buf = NULL;
    int ret = 0;
    char ipstr[INET6_ADDRSTRLEN];
    lthread_detach();

    inet_ntop(AF_INET, &cli_info->peer_addr.sin_addr, ipstr, INET_ADDRSTRLEN);
    printf("Accepted connection on IP %s\n", ipstr);

    if ((buf = malloc(1024)) == NULL)
        return;

    /* read data from client or timeout in 5 secs */
    ret = lthread_recv(cli_info->fd, buf, 1024, 0, 5000);

    /* did we timeout before the user has sent us anything? */
    if (ret == -2) {
        lthread_close(cli_info->fd);
        free(buf);
        free(arg);
        return;
    }

    /*
     * Run an expensive computation without blocking other lthreads.
     * lthread_compute_begin() will yield http_serv coroutine and resumes
     * it in a compute scheduler that runs in a pthread. If a compute scheduler
     * is already available and free it will be used otherwise a compute scheduler
     * is created and launched in a new pthread. After the compute scheduler
     * resumes the lthread it will wait 60 seconds for a new job and dies after 60
     * of inactivity.
     */
    lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(35);
    lthread_compute_end();

    /* reply back to user */
    lthread_send(cli_info->fd, reply, strlen(reply), 0);
    lthread_close(cli_info->fd);
    free(buf);
    free(arg);
}

void
listener(lthread_t *lt, void *arg)
{
    int cli_fd = 0;
    int lsn_fd = 0;
    int opt = 1;
    int ret = 0;
    struct sockaddr_in peer_addr = {};
    struct   sockaddr_in sin = {};
    socklen_t addrlen = sizeof(peer_addr);
    lthread_t *cli_lt = NULL;
    cli_info_t *cli_info = NULL;
    char ipstr[INET6_ADDRSTRLEN];
    lthread_detach();

    DEFINE_LTHREAD;

    /* create listening socket */
    lsn_fd = lthread_socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (lsn_fd == -1)
        return;

    if (setsockopt(lsn_fd, SOL_SOCKET, SO_REUSEADDR, &opt,sizeof(int)) == -1)
        perror("failed to set SOREUSEADDR on socket");

    sin.sin_family = PF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(3128);

    /* bind to the listening port */ 
    ret = bind(lsn_fd, (struct sockaddr *)&sin, sizeof(sin));
    if (ret == -1) {
        perror("Failed to bind on port 3128");
        return;
    }

    printf("Starting listener on 3128\n");

    listen(lsn_fd, 128);

    while (1) {
        /* block until a new connection arrives */
        cli_fd = lthread_accept(lsn_fd, (struct sockaddr*)&peer_addr, &addrlen);
        if (cli_fd == -1) {
            perror("Failed to accept connection");
            return;
        }

        if ((cli_info = malloc(sizeof(cli_info_t))) == NULL) {
            close(cli_fd);
            continue;
        }
        cli_info->peer_addr = peer_addr;
        cli_info->fd = cli_fd;
        /* launch a new lthread that takes care of this client */
        ret = lthread_create(&cli_lt, http_serv, cli_info);
    }
}

int
main(int argc, char **argv)
{
    lthread_t *lt = NULL;

    lthread_create(&lt, listener, NULL);
    lthread_run();

    return 0;
}

```

TODOS
-----

- aio support
