#include "lthread.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void
write_pipe(void *arg)
{
    int *pipes = arg;
    char buf[100] = "Hello world!";
    printf("fd of 1 is %d\n", pipes[1]);
    lthread_write(pipes[1], buf, strlen(buf));
}

void
read_pipe(void *arg)
{
    int *pipes = arg;
    char buf[100];
    printf("fd of 0 is %d\n", pipes[0]);
    lthread_read(pipes[0], buf, 100, 0);
    printf("read %s\n", buf);
}

int
main(int argc, char **argv)
{
	lthread_t *lt = NULL;
    int pipes[2];
    lthread_pipe(pipes);

	lthread_create(&lt, read_pipe, pipes);
	lthread_create(&lt, write_pipe, pipes);
	lthread_run();

	return 0;
}
