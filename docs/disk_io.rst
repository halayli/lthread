Disk IO
=======

The way disk async functions are implemented in lthread is by using a native
worker thread in the background to execute the actual read/write calls to disk.
When an lthread calls :c:func:`lthread_io_read()` or :c:func:`lthread_io_write`
a job is put on a queue for the native thread to pick up and the actual lthread
yields until the read/write is done.

Use :c:func:`lthread_io_read()` or :c:func:`lthread_io_write` when
fd is a file descriptor to a file.

lthread_io_read
---------------
.. c:function:: ssize_t lthread_io_read(int fd, void *buf, size_t nbytes);

    An async version of read(2) for disk IO.

lthread_io_write
----------------
.. c:function:: ssize_t lthread_io_write(int fd, const void *buf, size_t buf_len)

    An async version of write(2) for disk IO.


lthread_sendfile
-----------------
.. c:function:: int lthread_sendfile(int fd, int s, off_t offset, size_t nbytes, struct sf_hdtr *hdtr)

    An lthread version of sendfile(2). `man 2 sendfile` for more details.

.. note:: Available on FreeBSD only.
