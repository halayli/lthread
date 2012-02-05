#include <lthread.h>
#include <stdio.h>
#include <sys/time.h>

unsigned long long int
fibonacci(unsigned long long int n)
{
    if (n == 0)
        return 0;
     if (n == 1)
        return 1;

     return fibonacci(n - 1) + fibonacci(n - 2);
}

void
a(lthread_t *lt, void *x)
{
    int i = 100;
    unsigned long long int ret = 0, source = 40;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in a\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(2000);
        //gettimeofday(&t2, NULL);
        //printf("a (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(source);
    	lthread_compute_end();
	printf("[a] %dth Fibonocci: %d\n", source, ret);
    }
    printf("a is exiting\n");
}

void
b(lthread_t *lt, void *x)
{
    int i = 100;
    unsigned long long int ret = 0, source = 40;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in b\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(2000);
        //gettimeofday(&t2, NULL);
        //printf("a (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(source);
    	lthread_compute_end();
	printf("[b] %dth Fibonocci: %d\n", source, ret);
    }
    printf("b is exiting\n");
}

void
c(lthread_t *lt, void *x)
{
    int i = 100;
    unsigned long long int ret = 0, source = 40;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in c\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(2000);
        //gettimeofday(&t2, NULL);
        //printf("a (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(source);
    	lthread_compute_end();
	printf("[c] %dth Fibonocci: %d\n", source, ret);
    }
    printf("c is exiting\n");
}

void
d(lthread_t *lt, void *x)
{
    int i = 100;
    unsigned long long int ret = 0, source = 40;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in d\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(2000);
        //gettimeofday(&t2, NULL);
        //printf("a (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(source);
    	lthread_compute_end();
	printf("[d] %dth Fibonocci: %d\n", source, ret);
    }
    printf("d is exiting\n");
}


void
e(lthread_t *lt, void *x)
{
    int i = 100;
    unsigned long long int ret = 0, source = 40;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in e\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(2000);
        //gettimeofday(&t2, NULL);
        //printf("a (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(source);
    	lthread_compute_end();
	printf("[e] %dth Fibonocci: %d\n", source, ret);
    }
    printf("e is exiting\n");
}


void
f(lthread_t *lt, void *x)
{
    int i = 100;
    unsigned long long int ret = 0, source = 40;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in f\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(2000);
        //gettimeofday(&t2, NULL);
        //printf("a (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(source);
    	lthread_compute_end();
	printf("[f] %dth Fibonocci: %d\n", source, ret);
    }
    printf("f is exiting\n");
}


void
g(lthread_t *lt, void *x)
{
    int i = 100;
    unsigned long long int ret = 0, source = 40;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in g\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(2000);
        //gettimeofday(&t2, NULL);
        //printf("a (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(source);
    	lthread_compute_end();
	printf("[g] %dth Fibonocci: %d\n", source, ret);
    }
    printf("g is exiting\n");
}


void
h(lthread_t *lt, void *x)
{
    int i = 100;
    unsigned long long int ret = 0, source = 40;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in h\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(2000);
        //gettimeofday(&t2, NULL);
        //printf("a (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(source);
    	lthread_compute_end();
	printf("[h] %dth Fibonocci: %d\n", source, ret);
    }
    printf("h is exiting\n");
}
int
main(int argc, char **argv)
{
    lthread_t *lt = NULL;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    gettimeofday(&t1, NULL);
    lthread_create(&lt, a, NULL);
    lthread_create(&lt, b, NULL);
    lthread_create(&lt, c, NULL);
    lthread_create(&lt, d, NULL);
    lthread_create(&lt, e, NULL);
    lthread_create(&lt, f, NULL);
    lthread_create(&lt, g, NULL);
    lthread_create(&lt, h, NULL);
    lthread_join();
    gettimeofday(&t2, NULL);
    printf("Time elapsed is: %ld\n", t2.tv_sec - t1.tv_sec);
    return 0;
}

