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

  lthread.h
*/

#ifndef _LTHREAD_H_
#define _LTHREAD_H_ 

#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>

#define DEFINE_LTHREAD (lthread_set_funcname(__func__))

#ifndef _LTHREAD_INT_H_
struct _lthread;
struct _lthread_cond;
typedef struct _lthread lthread_t;
typedef struct _lthread_cond lthread_cond_t;
#endif

char    *lthread_summary();

int     lthread_create(lthread_t **new_lt, void *fun, void *arg);
void    lthread_destroy(lthread_t *lt);
void    lthread_join(void);
void    lthread_sleep(uint64_t msecs);
void    lthread_wakeup(lthread_t *lt);
int     lthread_cond_create(lthread_cond_t **c);
int     lthread_cond_wait(lthread_cond_t *c, uint64_t timeout);
void    lthread_cond_signal(lthread_cond_t *c);
int     lthread_init(size_t size);
void    *lthread_get_data(void);
void    lthread_set_data(void *data);
lthread_t *lthread_current();

/* socket related functions */
int     lthread_socket(int, int, int);
int     lthread_accept(int fd, struct sockaddr *, socklen_t *);
int     lthread_close(int fd);
void    lthread_set_funcname(const char *f);
uint64_t lthread_id();
int     lthread_connect(int fd, struct sockaddr *, socklen_t, uint32_t timeout);
ssize_t lthread_recv(int fd, void * buf, size_t buf_len, int flags, unsigned int timeout);
ssize_t lthread_send(int fd, const void *buf, size_t buf_len, int flags);
ssize_t lthread_writev(int fd, struct iovec *iov, int iovcnt);
#ifdef __FreeBSD__
int     lthread_sendfile(int fd, int s, off_t offset, size_t nbytes, struct sf_hdtr *hdtr);
#endif

#endif
