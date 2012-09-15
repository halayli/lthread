#include <lthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

void a(void *x);
void b(void *x);

void
b(void *x)
{
    DEFINE_LTHREAD;
    void *buf = malloc(100);
    printf("malloc is %p\n", buf);
	printf("b is running\n");
    lthread_compute_begin();
        sleep(1);
    lthread_compute_end();
	printf("b is exiting\n");
}

void *ptr = NULL;
void
a(void *x)
{
	lthread_t *lt_new = NULL;

    DEFINE_LTHREAD;
    lthread_detach();

    printf("a is running\n");
    lthread_create(&lt_new, b, NULL);
    lthread_join(lt_new, &ptr, 0);
    printf("a is done joining on b. ptr %p value %p\n", &ptr, ptr);
}


int
main(int argc, char **argv)
{
	lthread_t *lt = NULL;

	lthread_create(&lt, a, NULL);
	lthread_run();

	return 0;
}
