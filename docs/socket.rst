Socket
======

lthread_socket
--------------
.. c:function:: int lthread_socket(int domain, int type, int protocol)

    Creates a new socket(2) and sets it to non-blocking.

    Parameters can be found in `man socket`.

lthread_pipe
------------
.. c:function:: int lthread_pipe(int fildes[2])

    lthread version of pipe(2), with socket set to non-blocking.

lthread_accept
--------------
.. c:function:: int lthread_accept(int fd, struct sockaddr *, socklen_t *)

    lthread version of accept(2). `man 2 accept` for more details.

lthread_close
-------------
.. c:function:: int lthread_close(int fd)

    lthread version of close(2). `man 2 close` for more details.

lthread_connect
---------------
.. c:function:: int lthread_connect(int fd, struct sockaddr *, socklen_t, uint64_t timeout)

    lthread version of connect(2) with additional timeout parameter.

    :return: new fd > 0 on success, -1 on failure, -2 on timeout.

lthread_recv
------------
.. c:function:: ssize_t lthread_recv(int fd, void *buf, size_t buf_len, int flags, uint64_t timeout)

    lthread version of recv(2), with additional timeout parameter.

    :return: Returns number of bytes read, -1 on failure and -2 on timeout.

lthread_read
------------
.. c:function:: ssize_t lthread_read(int fd, void *buf, size_t length, uint64_t timeout)

    lthread version of read(2), with additional timeout parameter.

    :return: Returns number of bytes read, -1 on failure and -2 on timeout.

lthread_readline
----------------
.. c:function:: ssize_t lthread_readline(int fd, char **buf, size_t max, uint64_t timeout)

    Keeps reading from fd until it hits a \n or `max` bytes.

    :return: Number of bytes read, -1 on failure and -2 on timeout.

lthread_recv_exact
------------------
.. c:function:: ssize_t lthread_recv_exact(int fd, void *buf, size_t buf_len, int flags, uint64_t timeout)

    Blocks until exact number of bytes are read.

    :return: Number of bytes read, -1 on failure and -2 on timeout.

lthread_read_exact
------------------
.. c:function:: ssize_t lthread_read_exact(int fd, void *buf, size_t length, uint64_t timeout)

    Blocks until exact number of bytes are read.

    :return: Number of bytes read, -1 on failure and -2 on timeout.

lthread_recvmsg
---------------
.. c:function:: ssize_t lthread_recvmsg(int fd, struct msghdr *message, int flags, uint64_t timeout)

    lthread version of recvmsg(2). `man 2 recvmsg` for more details.

    :return: Returns number of bytes read, -1 on failure and -2 on timeout.

lthread_recvfrom
----------------
.. c:function:: ssize_t lthread_recvfrom(int fd, void *buf, size_t length, int flags,\
                                         struct sockaddr *address,\
                                         socklen_t *address_len, uint64_t timeout)

    lthread version of recvfrom(2). `man 2 recvfrom` for more details.

    :return: Returns number of bytes read, -1 on failure and -2 on timeout.

lthread_send
--------------
.. c:function:: ssize_t lthread_send(int fd, const void *buf, size_t buf_len, int flags)

    lthread version of send(2). `man 2 send` for more details.

lthread_write
-------------
.. c:function:: ssize_t lthread_write(int fd, const void *buf, size_t buf_len)

    lthread version of write(2). `man 2 write` for more details.

lthread_sendmsg
---------------
.. c:function:: ssize_t lthread_sendmsg(int fd, const struct msghdr *message, int flags)

    lthread version of sendmsg(2). `man 2 sendmsg` for more details.

lthread_sendto
--------------
.. c:function:: ssize_t lthread_sendto(int fd, const void *buf, size_t length,\
                                       int flags, const struct sockaddr *dest_addr,\
                                       socklen_t dest_len)

    lthread version of sendto(2). `man 2 sendto` for more details.

lthread_writev
--------------
.. c:function:: ssize_t lthread_writev(int fd, struct iovec *iov, int iovcnt)

    lthread version of writev(2). `man 2 writev` for more details.

lthread_wait_read
-----------------
.. c:function:: int lthread_wait_read(int fd, int timeout_ms)

    Waits for an fd to become readable.

    :return: 0 on success, or -2 on timeout.


lthread_wait_write
------------------
.. c:function:: int lthread_wait_write(int fd, int timeout_ms)

    Waits for an fd to become writable.

    :return: 0 on success, or -2 on timeout.
