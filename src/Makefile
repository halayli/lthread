src = lthread.c lthread_socket.c lthread_sched.c lthread_compute.c \
    ../common/rbtree.c ../common/time.c poller.c
gccflags = "-g"

lthread.o: $(src)
	gcc -Werror -fPIC -g -I ../ $(gccflags) -Wall -c $(src)
	gcc -Werror -lpthread -O2 $(gccflags) -shared -W1,-soname,liblthread.so.1 -o liblthread.so *.o
	ar -cvq liblthread.a *.o

install: lthread.o
	cp liblthread.so* /usr/lib/
	cp liblthread.a /usr/lib/
	cp lthread.h /usr/local/include/

test: lthread.o
	gcc -g rbtree.o time.o poller.o lthread_sched.o lthread.o lthread_unit_test.c -o test_lthread

compute_test: lthread.o
	gcc -lpthread -g rbtree.o time.o poller.o lthread_sched.o lthread.o lthread_compute.o lthread_unit_test_compute.c -o test_lthread

uninstall: 
	rm /usr/lib/liblthread.so
	rm /usr/lib/liblthread.a

clean: 
	rm -f *.o *.so *.a
