#include "lthread.h"
#include <sys/time.h>
#include <stdio.h>

void a(void *x);

void
a(void *x)
{
	int i = 3;
	struct timeval t1 = {0, 0};
	struct timeval t2 = {0, 0};
    lthread_detach();
    int sleep_for = 2000;
	while (i--) {
		gettimeofday(&t1, NULL);
		lthread_sleep(sleep_for);
		gettimeofday(&t2, NULL);
		printf("a (%d): elapsed is: %lf, slept is %d\n", i,
            ((t2.tv_sec * 1000.0) + t2.tv_usec /1000.0) -
            ((t1.tv_sec * 1000.0) + t1.tv_usec/1000.0), sleep_for);
	}
}


int
main(int argc, char **argv)
{
	lthread_t *lt = NULL;

	lthread_create(&lt, a, NULL);
	lthread_run();

	return 0;
}
