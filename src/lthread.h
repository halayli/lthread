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
 * lthread.h
 */


#ifndef LTHREAD_H
#define LTHREAD_H

#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>

#define DEFINE_LTHREAD (lthread_set_funcname(__func__))

#ifndef LTHREAD_INT_H
struct lthread;
struct lthread_cond;
typedef struct lthread lthread_t;
typedef struct lthread_cond lthread_cond_t;
#endif

char    *lthread_summary();

typedef void (*lthread_func)(void *);
#ifdef __cplusplus
extern "C" {
#endif

int     lthread_create(lthread_t **new_lt, lthread_func, void *arg);
void    lthread_cancel(lthread_t *lt);
void    lthread_run(void);
int     lthread_join(lthread_t *lt, void **ptr, uint64_t timeout);
void    lthread_detach(void);
void    lthread_exit(void *ptr);
void    lthread_sleep(uint64_t msecs);
void    lthread_wakeup(lthread_t *lt);
int     lthread_cond_create(lthread_cond_t **c);
int     lthread_cond_wait(lthread_cond_t *c, uint64_t timeout);
void    lthread_cond_signal(lthread_cond_t *c);
void    lthread_cond_broadcast(lthread_cond_t *c);
int     lthread_init(size_t size);
void    *lthread_get_data(void);
void    lthread_set_data(void *data);
lthread_t *lthread_current();

/* socket related functions */
int     lthread_socket(int, int, int);
int     lthread_pipe(int fildes[2]);
int     lthread_accept(int fd, struct sockaddr *, socklen_t *);
int     lthread_close(int fd);
void    lthread_set_funcname(const char *f);
uint64_t lthread_id();
int     lthread_connect(int fd, struct sockaddr *, socklen_t, uint64_t timeout);
ssize_t lthread_recv(int fd, void *buf, size_t buf_len, int flags,
    uint64_t timeout);
ssize_t lthread_read(int fd, void *buf, size_t length, uint64_t timeout);
ssize_t lthread_readline(int fd, char **buf, size_t max, uint64_t timeout);
ssize_t lthread_recv_exact(int fd, void *buf, size_t buf_len, int flags,
    uint64_t timeout);
ssize_t lthread_read_exact(int fd, void *buf, size_t length, uint64_t timeout);
ssize_t lthread_recvmsg(int fd, struct msghdr *message, int flags,
    uint64_t timeout);
ssize_t lthread_recvfrom(int fd, void *buf, size_t length, int flags,
    struct sockaddr *address, socklen_t *address_len, uint64_t timeout);

ssize_t lthread_send(int fd, const void *buf, size_t buf_len, int flags);
ssize_t lthread_write(int fd, const void *buf, size_t buf_len);
ssize_t lthread_sendmsg(int fd, const struct msghdr *message, int flags);
ssize_t lthread_sendto(int fd, const void *buf, size_t length, int flags,
    const struct sockaddr *dest_addr, socklen_t dest_len);
ssize_t lthread_writev(int fd, struct iovec *iov, int iovcnt);
#ifdef __FreeBSD__
int     lthread_sendfile(int fd, int s, off_t offset, size_t nbytes,
    struct sf_hdtr *hdtr);
#endif
ssize_t lthread_io_write(int fd, void *buf, size_t nbytes);
ssize_t lthread_io_read(int fd, void *buf, size_t nbytes);

int lthread_compute_begin(void);
void lthread_compute_end(void);
#ifdef __cplusplus
}
#endif

#endif
