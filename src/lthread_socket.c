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

  lthread_socket.c
*/

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "lthread_int.h"

#if defined(__FreeBSD__) || defined(__APPLE__)
    #define FLAG
#else
    #define FLAG | MSG_NOSIGNAL
#endif

#define LTHREAD_RECV(x, y)                                  \
x {                                                         \
    ssize_t ret = 0;                                        \
    lthread_t *lt = lthread_get_sched()->current_lthread;   \
    while (1) {                                             \
        if (lt->state & bit(LT_FDEOF))                      \
            return -1;                                      \
        ret = y;                                            \
        if (ret == -1 && errno != EAGAIN)                   \
            return -1;                                      \
        if ((ret == -1 && errno == EAGAIN)) {               \
            if (timeout)                                    \
                _sched_lthread(lt, timeout);                \
            _lthread_wait_for(lt, fd, LT_READ);             \
            if (lt->state & bit(LT_EXPIRED))                \
                return -2;                                  \
        }                                                   \
        if (ret >= 0)                                       \
            return ret;                                     \
    }                                                       \
}                                                           \

#define LTHREAD_SEND(x, y)                                  \
x {                                                         \
    ssize_t ret = 0;                                        \
    ssize_t sent = 0;                                       \
    lthread_t *lt = lthread_get_sched()->current_lthread;   \
    while (sent != length) {                                \
        if (lt->state & bit(LT_FDEOF))                      \
            return -1;                                      \
        ret = y;                                            \
        if (ret == 0)                                       \
            return sent;                                    \
        if (ret > 0)                                        \
            sent += ret;                                    \
        if (ret == -1 && errno != EAGAIN)                   \
            return -1;                                      \
        if (ret == -1 && errno == EAGAIN)                   \
            _lthread_wait_for(lt, fd, LT_WRITE);            \
    }                                                       \
    return sent;                                            \
}                                                           \

#define LTHREAD_SEND_ONCE(x, y)                             \
x {                                                         \
    ssize_t ret = 0;                                        \
    lthread_t *lt = lthread_get_sched()->current_lthread;   \
    while (1) {                                             \
        if (lt->state & bit(LT_FDEOF))                      \
            return -1;                                      \
        ret = y;                                            \
        if (ret >= 0)                                       \
            return ret;                                     \
        if (ret == -1 && errno != EAGAIN)                   \
            return -1;                                      \
        if (ret == -1 && errno == EAGAIN)                   \
            _lthread_wait_for(lt, fd, LT_WRITE);            \
    }                                                       \
}                                                           \

const struct linger nolinger = { .l_onoff = 1, .l_linger = 1 };

int
lthread_accept(int fd, struct sockaddr *addr, socklen_t *len)
{
    int ret = -1;
    lthread_t *lt = lthread_get_sched()->current_lthread;

    while (1) {
        ret = accept(fd, addr, len);
        if (ret == -1 && 
            (errno == ENFILE || 
            errno == EWOULDBLOCK ||
            errno == EMFILE)) {
            _lthread_wait_for(lt, fd, LT_READ);
            continue;
        }

        if (ret > 0)
            break;

        if (ret == -1 && errno == ECONNABORTED)  {
            perror("Cannot accept connection");
            continue;
        }

        if (ret == -1 && errno != EWOULDBLOCK) {
            perror("Cannot accept connection");
            return -1;
        }

    }

#ifndef __FreeBSD__
    if ((fcntl(ret, F_SETFL, O_NONBLOCK)) == -1) {
        close(fd);
        perror("Failed to set socket properties");
        return -1;
    }
#endif

    return ret;
}

int
lthread_close(int fd)
{
    lthread_t *lt = lthread_get_sched()->current_lthread;
    
    close(fd);
    _desched_lthread(lt);
    clear_rd_wr_state(lt);

    return 0;
}

int
lthread_socket(int domain, int type, int protocol)
{
    int fd;
#if defined(__FreeBSD__) || defined(__APPLE__)
    int set = 1;
#endif

    if ((fd = socket(domain, type, protocol)) == -1) {
        perror("Failed to create a new socket");
        return -1;
    }

    if ((fcntl(fd, F_SETFL, O_NONBLOCK)) == -1) {
        close(fd);
        perror("Failed to set socket properties");
        return -1;
    }

#if defined(__FreeBSD__) || defined(__APPLE__)
    if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(int)) == -1) {
        close(fd);
        perror("Failed to set socket properties");
        return -1;
    }
#endif

    return fd;
}

ssize_t
lthread_recv_exact(int fd, void *buffer, size_t length, int flags, uint64_t timeout)
{
    ssize_t ret = 0;
    ssize_t recvd = 0;
    lthread_t *lt = lthread_get_sched()->current_lthread;

    while (recvd != length) {
        if (lt->state & bit(LT_FDEOF))
            return -1;

        ret = recv(fd, buffer + recvd, length - recvd, flags FLAG);

        if (ret == 0)
            return recvd;

        if (ret > 0)
            recvd += ret;

        if (ret == -1 && errno != EAGAIN)
            return -1;

        if ((ret == -1 && errno == EAGAIN)) {
            if (timeout)
                _sched_lthread(lt, timeout);
            _lthread_wait_for(lt, fd, LT_READ);
            if (lt->state & bit(LT_EXPIRED))
                return -2;
        }
    }

    return recvd;
}

LTHREAD_RECV(
    ssize_t lthread_recv(int fd, void *buffer, size_t length, int flags, uint64_t timeout),
    recv(fd, buffer, length, flags FLAG)
)

LTHREAD_RECV(
    ssize_t lthread_recvmsg(int fd, struct msghdr *message, int flags, uint64_t timeout),
    recvmsg(fd, message, flags FLAG)
)

LTHREAD_RECV(
    ssize_t lthread_recvfrom(int fd, void *buffer, size_t length, int flags,
        struct sockaddr *address, socklen_t *address_len, uint64_t timeout),
    recvfrom(fd, buffer, length, flags FLAG, address, address_len)
)

LTHREAD_SEND(
    ssize_t lthread_send(int fd, const void *buffer, size_t length, int flags),
    send(fd,((char *)buffer) + sent, length - sent, flags FLAG)
)

LTHREAD_SEND_ONCE(
    ssize_t lthread_sendmsg(int fd, const struct msghdr *message, int flags),
    sendmsg(fd, message, flags FLAG)
)

LTHREAD_SEND_ONCE(
    ssize_t lthread_sendto(int fd, const void *buffer, size_t length, int flags,
        const struct sockaddr *dest_addr, socklen_t dest_len),
    sendto(fd, buffer, length, flags FLAG, dest_addr, dest_len)
)

int
lthread_connect(int fd, struct sockaddr *name, socklen_t namelen, uint64_t timeout)
{

    int ret = 0;
    lthread_t *lt = lthread_get_sched()->current_lthread;

    while (1) {
        ret = connect(fd, name, namelen);
        if (ret == 0)
            break;
        if (ret == -1 && (errno == EAGAIN || 
            errno == EWOULDBLOCK ||
            errno == EINPROGRESS)) {
            if (timeout)
                _sched_lthread(lt, timeout);
            _lthread_wait_for(lt, fd, LT_WRITE);
            if (lt->state & bit(LT_EXPIRED))
                return -2;
            
            ret = 0;
            break;
        } else {
            break;
        }
    }

    return ret;
}

ssize_t
lthread_writev(int fd, struct iovec *iov, int iovcnt)
{
    ssize_t total = 0;
    int iov_index = 0;
    lthread_t *lt = lthread_get_sched()->current_lthread;

    do {
        ssize_t n = writev(fd, iov + iov_index, iovcnt - iov_index);
        if (n > 0) {
            int i = 0;
            total += n;
            for (i = iov_index; i < iovcnt && n > 0; i++) {
                if (n < iov[i].iov_len) {
                    iov[i].iov_base += n;
                    iov[i].iov_len -= n;
                    n = 0;
                } else {
                    n -= iov[i].iov_len;
                    iov_index++;
                }
            }
        } else if (-1 == n && EAGAIN == errno) {
            _lthread_wait_for(lt, fd, LT_WRITE);
        } else {
            return n;
        }
    } while (iov_index < iovcnt);
    return total;
}

#ifdef __FreeBSD__
int
lthread_sendfile(int fd, int s, off_t offset, size_t nbytes,
    struct sf_hdtr *hdtr)
{

    off_t sbytes = 0;
    int ret = 0;
    lthread_t *lt = lthread_get_sched()->current_lthread;

    do {
        ret = sendfile(fd, s, offset, nbytes, hdtr, &sbytes, 0);

        if (ret == 0)
            return 0;

        if (sbytes)
            offset += sbytes;

        sbytes = 0;

        if (ret == -1 && EAGAIN == errno)
            _lthread_wait_for(lt, s, LT_WRITE);
        else if (ret == -1)
            return -1;

    } while (1);
}
#endif
