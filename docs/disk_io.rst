Disk IO
=======

lthread_io_read
---------------
.. c:function:: ssize_t lthread_io_read(int fd, void *buf, size_t nbytes);

    An async version of read(2) for disk IO.

lthread_io_write
----------------
.. c:function:: ssize_t lthread_io_write(int fd, const void *buf, size_t buf_len)

    An async version of write(2) for disk IO.

