#include "lthread.h"
#include <sys/time.h>
#include <stdio.h>

void a(void *x);
void b(void *x);
void c(void *x);

void
b(void *x)
{
	int i = 5;
	struct timeval t1 = {0, 0};
	struct timeval t2 = {0, 0};
    lthread_detach();
	//printf("I am in b\n");
	while (i--) {
		gettimeofday(&t1, NULL);
		lthread_sleep(3000);
		gettimeofday(&t2, NULL);
		printf("b (%d): elapsed is: %ld, slept is %d\n",i ,t2.tv_sec - t1.tv_sec, 3000);
	}
	printf("b is exiting\n");
	printf("I am b, after yield\n");
}

void
c(void *x)
{
	int i = 4;
    lthread_t *lt_new = NULL;
	struct timeval t1 = {0, 0};
	struct timeval t2 = {0, 0};
    lthread_detach();
	DEFINE_LTHREAD;
	//printf("I am in c\n");
	while (i--) {
		gettimeofday(&t1, NULL);
		lthread_sleep(100);
		gettimeofday(&t2, NULL);
	printf("c (%d): elapsed is: %ld, slept is %d\n",i ,t2.tv_sec - t1.tv_sec, 100);
		lthread_create(&lt_new, b, NULL);
	}
	printf("I am c, after yield\n");
}

void
a(void *x)
{
	int i = 3;
	lthread_t *lt_new = NULL;
	//DEFINE_LTHREAD(lt);
	struct timeval t1 = {0, 0};
	struct timeval t2 = {0, 0};
    lthread_detach();
	//printf("I am in a\n");
	while (i--) {
		gettimeofday(&t1, NULL);
		lthread_sleep(2000);
		gettimeofday(&t2, NULL);
		printf("a (%d): elapsed is: %ld, slept is %d\n",i ,t2.tv_sec - t1.tv_sec, 5000);
		lthread_create(&lt_new, c, NULL);
		//printf("%s\n", lthread_summary(lthread_get_sched(lt)));
	}
	printf("a is exiting\n");
	printf("I am a, after yield\n");
}


int
main(int argc, char **argv)
{
	lthread_t *lt = NULL;

	lthread_create(&lt, a, NULL);
	lthread_run();

	return 0;
}
