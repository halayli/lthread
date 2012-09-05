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
a(void *x)
{
    int i = 3;
    unsigned long long int ret = 0;
    DEFINE_LTHREAD;
    lthread_detach();
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
        ret = fibonacci(42);
    	lthread_compute_end();
	printf("[a] 42th Fibonocci: %d\n", ret);
    }
    printf("a is exiting\n");
}

void
b(void *x)
{
    int i = 8;
    unsigned long long int ret = 0;
    DEFINE_LTHREAD;
    lthread_detach();
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in b\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(1000);
        //gettimeofday(&t2, NULL);
        //printf("b (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(43);
    	lthread_compute_end();
	printf("[b] 43th Fibonocci: %d\n", ret);
    }
    printf("b is exiting\n");
}

void
c(void *x)
{
    int i = 15;
    unsigned long long int ret = 0;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in c\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(2000);
        //gettimeofday(&t2, NULL);
        //printf("c (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(44);
    	lthread_compute_end();
	printf("[c] 44th Fibonocci: %d\n", ret);
    }
    printf("c is exiting\n");
}

void
d(void *x)
{
    int i = 21;
    unsigned long long int ret = 0;
    DEFINE_LTHREAD;
    lthread_detach();
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in d\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(1000);
        //gettimeofday(&t2, NULL);
        //printf("d (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(45);
    	lthread_compute_end();
	printf("[d] 45th Fibonocci: %d\n", ret);
    }
    printf("d is exiting\n");
}

void
e(void *x)
{
    int i = 26;
    unsigned long long int ret = 0;
    DEFINE_LTHREAD;
    lthread_detach();
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in e\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(2000);
        //gettimeofday(&t2, NULL);
        //printf("e (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(46);
    	lthread_compute_end();
	printf("[e] 46th Fibonocci: %d\n", ret);
    }
    printf("e is exiting\n");
}

void
f(void *x)
{
    int i = 31;
    unsigned long long int ret = 0;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in f\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(1000);
        //gettimeofday(&t2, NULL);
        //printf("e (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(47);
    	lthread_compute_end();
	printf("[e] 47th Fibonocci: %d\n", ret);
    }
    printf("f is exiting\n");
}

void
g(void *x)
{
    int i = 36;
    unsigned long long int ret = 0;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in g\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(2000);
        //gettimeofday(&t2, NULL);
        //printf("g (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(48);
    	lthread_compute_end();
	printf("[g] 48th Fibonocci: %d\n", ret);
    }
    printf("g is exiting\n");
}

void
h(void *x)
{
    int i = 45;
    unsigned long long int ret = 0;
    DEFINE_LTHREAD;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    printf("I am in h\n");

    while (i--) {
        //gettimeofday(&t1, NULL);
        //lthread_sleep(1000);
        //gettimeofday(&t2, NULL);
        //printf("h (%d): elapsed is: %ld\n",i ,t2.tv_sec - t1.tv_sec);
	lthread_compute_begin();
        /* make an expensive call without blocking other coroutines */
        ret = fibonacci(49);
    	lthread_compute_end();
	printf("[h] 49th Fibonocci: %d\n", ret);
    }
    printf("h is exiting\n");
}

int
main(int argc, char **argv)
{
    lthread_t *lt = NULL;

    lthread_create(&lt, a, NULL);
    lthread_create(&lt, b, NULL);
    lthread_create(&lt, c, NULL);
    lthread_create(&lt, d, NULL);
    lthread_create(&lt, e, NULL);
    lthread_create(&lt, f, NULL);
    lthread_create(&lt, g, NULL);
    lthread_create(&lt, h, NULL);
    lthread_run();

    return 0;
}

