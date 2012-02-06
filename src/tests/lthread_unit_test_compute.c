#include "lthread.h"
#include <sys/time.h>
#include <stdio.h>

void a(lthread_t *lt, void *x);

void
test(char *p)
{
    printf("char *p is %p, %c\n", p, *p);
    *p = 'b';
}

void
a(lthread_t *lt ,void *arg)
{
	int i = 3;
	struct timeval t1 = {0, 0};
	struct timeval t2 = {0, 0};
    char x = 'a';
    char *y = &x;
    DEFINE_LTHREAD;
    lthread_detach();

    printf("char *p is %p, %c\n", &x, x);
	while (i--) {
        lthread_compute_begin();
            test(y);
            printf("I am A in a compute thread \n");
        lthread_compute_end();
        printf("char *p became %p, %c\n", &x, x);
        printf("A going for a nap\n");
        lthread_sleep(3000);
        printf("A after sleep: %d\n", i);
	}
	printf("a is exiting\n");
}

void
b(lthread_t *lt ,void *x)
{
	int i = 3;
	struct timeval t1 = {0, 0};
	struct timeval t2 = {0, 0};
    DEFINE_LTHREAD;
    lthread_detach();

    lthread_sleep(1000);
	while (i--) {
    printf(" I am B before begin\n");
    lthread_compute_begin();
        printf("I am B in a compute thread \n");
        sleep(10);
    lthread_compute_end();
    printf("B after  sleep\n");
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
