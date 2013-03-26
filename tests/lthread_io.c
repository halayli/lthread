#include "lthread.h"
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

void a(void *x);
char line[] = "this is an lthread io test\n";

void
a(void *arg)
{
    int i, x;
    i = x = 50;
    struct timeval t1 = {0, 0};
    struct timeval t2 = {0, 0};
    lthread_detach();
    int fd = 0;
    double total = 0;

    fd = open("/tmp/lthread_io.txt", O_CREAT | O_APPEND | O_WRONLY, 0640);
        lthread_io_write(fd, line, sizeof(line));
    while (i--) {
        gettimeofday(&t1, NULL);
        lthread_io_write(fd, line, sizeof(line));
        //write(fd, line, sizeof(line));
        gettimeofday(&t2, NULL);
        total += ((t2.tv_sec * 1000000) + t2.tv_usec) -
             ((t1.tv_sec * 1000000) + t1.tv_usec);
        printf("elapsed is: %ld\n",
            ((t2.tv_sec * 1000000) + t2.tv_usec) -
            ((t1.tv_sec * 1000000) + t1.tv_usec));

    }
    printf("io_write took %lf msec (avg: %lf usec) to write %d lines\n", total, total / (double)x, x);
}

void
b(void *x)
{
    int i = 5;
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
    //lthread_create(&lt, b, NULL);
    lthread_run();

    return 0;
}
