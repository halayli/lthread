#include "lthread.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

/*
 * this test is analogous to lthread_fdeof_read.c, but for write function
 * even though the error is encountered only for read-calls, write test was made
 * just in case
 */


bool PASSED = false;
const char * const PHRASE = "Hello world!";

void
write_pipe(void *arg)
{
	int *pipes = arg;
	int ret;

	for (int i = 1; i <= 3; i += 2) {
		ret = lthread_write(pipes[i], PHRASE, strlen(PHRASE) + 1);
		printf("fd of %d is %d, written %d %s\n", i, pipes[i], ret, ret < 0 ? strerror(errno) : "");
	}
}

void
read_pipe(void *arg)
{
	int *pipes = arg;
	char buf[100];
	int ret;

	printf("closing fd of 0 %d\n", pipes[0]); /* first write must fail */
	lthread_close(pipes[0]);
	ret = lthread_read(pipes[2], buf, sizeof(buf), 0); /* second write must succeed */
	lthread_close(pipes[2]);
	if (ret) {
		printf("fd %d, read '%s' [%d]\n", pipes[2], buf, ret);
	} else {
		printf("read from fd %d failed %d\n", pipes[2], ret);
	}

	if (strcmp(buf, PHRASE) == 0)
		PASSED = true;
}

int
main(int argc, char **argv)
{
	struct sigaction act = { 0 };
	act.sa_handler = SIG_IGN;

	sigaction(SIGPIPE, &act, NULL);

	lthread_t *lt = NULL;
	int pipes[4];
	assert(lthread_pipe(pipes) == 0);
	assert(lthread_pipe(pipes + 2) == 0);

	lthread_create(&lt, read_pipe, pipes);
	lthread_create(&lt, write_pipe, pipes);
	lthread_run();

	return PASSED != true;
}
