src = lthread.c lthread_socket.c lthread_sched.c lthread_compute.c \
    ../common/rbtree.c ../common/time.c poller.c
gccflags = "-g"

lthread.o: $(src)
	gcc -Werror -fPIC -g -I ../ $(gccflags) -Wall -c $(src)
	gcc -Werror -lpthread -O2 $(gccflags) -shared -Wl,-soname,liblthread.so.1 -o liblthread.so *.o
	ar -cvq liblthread.a *.o

install: lthread.o
	test -d "$(DESTDIR)$(libdir)" || mkdir -p "$(DESTDIR)$(libdir)"
	test -d "$(DESTDIR)$(includedir)" || mkdir -p "$(DESTDIR)$(includedir)"
	cp liblthread.so "$(DESTDIR)$(libdir)"
	cp lthread.h "$(DESTDIR)$(includedir)"

test: lthread.o
	gcc -lpthread -g rbtree.o time.o poller.o lthread_sched.o lthread.o lthread_unit_test.c -o test_lthread

compute_test: lthread.o
	gcc -lpthread -g rbtree.o time.o poller.o lthread_sched.o lthread.o lthread_compute.o lthread_unit_test_compute.c -o test_lthread

uninstall: 
	rm /usr/lib/liblthread.so
	rm /usr/lib/liblthread.a

clean: 
	rm -f *.o *.so *.a
