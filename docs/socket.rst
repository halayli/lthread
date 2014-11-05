Socket
======

lthread_socket
--------------
.. c:function:: int lthread_socket(int, int, int)

lthread_pipe
------------
.. c:function:: int lthread_pipe(int fildes[2])

lthread_accept
--------------
.. c:function:: int lthread_accept(int fd, struct sockaddr *, socklen_t *)

lthread_close
-------------
.. c:function:: int lthread_close(int fd)

lthread_connect
---------------
.. c:function:: int lthread_connect(int fd, struct sockaddr *, socklen_t, uint64_t timeout)

lthread_recv
------------
.. c:function:: ssize_t lthread_recv(int fd, void *buf, size_t buf_len, int flags, uint64_t timeout)

lthread_read
------------
.. c:function:: ssize_t lthread_read(int fd, void *buf, size_t length, uint64_t timeout)

lthread_readline
----------------
.. c:function:: ssize_t lthread_readline(int fd, char **buf, size_t max, uint64_t timeout)

lthread_recv_exact
------------------
.. c:function:: ssize_t lthread_recv_exact(int fd, void *buf, size_t buf_len, int flags, uint64_t timeout)

lthread_read_exact
------------------
.. c:function:: ssize_t lthread_read_exact(int fd, void *buf, size_t length, uint64_t timeout)

lthread_recvmsg
---------------
.. c:function:: ssize_t lthread_recvmsg(int fd, struct msghdr *message, int flags, uint64_t timeout)

lthread_recvfrom
----------------
.. c:function:: ssize_t lthread_recvfrom(int fd, void *buf, size_t length, int flags,\
                                         struct sockaddr *address,\
                                         socklen_t *address_len, uint64_t timeout)

lthread_send
--------------
.. c:function:: ssize_t lthread_send(int fd, const void *buf, size_t buf_len, int flags)


lthread_write
-------------
.. c:function:: ssize_t lthread_write(int fd, const void *buf, size_t buf_len)


lthread_sendmsg
---------------
.. c:function:: ssize_t lthread_sendmsg(int fd, const struct msghdr *message, int flags)

lthread_sendto
--------------
.. c:function:: ssize_t lthread_sendto(int fd, const void *buf, size_t length,\
                                       int flags, const struct sockaddr *dest_addr,\
                                       socklen_t dest_len)

lthread_writev
--------------
.. c:function:: ssize_t lthread_writev(int fd, struct iovec *iov, int iovcnt)

lthread_wait_read
-----------------
.. c:function:: int lthread_wait_read(int fd, int timeout_ms)

lthread_wait_write
------------------
.. c:function:: int lthread_wait_write(int fd, int timeout_ms)

