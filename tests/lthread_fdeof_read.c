#include "lthread.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

/*
 * test made after the bug was found:
 * lthread reads from the fd that was closed by the other side,
 * then next read from an alive fd will be a guaranteed failure
 */


bool PASSED = false;
const char * const PHRASE = "Hello world!";

void
write_pipe(void *arg)
{
	int *pipes = arg;
	printf("closing fd of 1 %d\n", pipes[1]);
	lthread_close(pipes[1]); /* first read must fail */
	printf("fd of 3 is %d\n", pipes[3]);
	lthread_write(pipes[3], PHRASE, strlen(PHRASE) + 1); /* second read must succeed */
	lthread_close(pipes[3]);
}

void
read_pipe(void *arg)
{
	int *pipes = arg;
	char buf[100];
	int ret;

	for (int i = 0; i <= 2; i += 2) {
		ret = lthread_read(pipes[i], buf, sizeof(buf), 0);
		lthread_close(pipes[i]);
		if (ret) {
			printf("fd %d, read '%s' [%d]\n", pipes[i], buf, ret);
		} else {
			printf("read from fd %d failed %d\n", pipes[i], ret);
			continue;
		}

		if (strcmp(buf, PHRASE) == 0)
			PASSED = true;
	}
}

int
main(int argc, char **argv)
{
	lthread_t *lt = NULL;
	int pipes[4];
	assert(lthread_pipe(pipes) == 0);
	assert(lthread_pipe(pipes + 2) == 0);

	lthread_create(&lt, read_pipe, pipes);
	lthread_create(&lt, write_pipe, pipes);
	lthread_run();

	return PASSED != true;
}
