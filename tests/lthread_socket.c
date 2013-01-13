#include "lthread.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

struct conn {
    int fd;
};

void
socket_client_server_reader(void *cli_fd)
{
    int fd = ((struct conn *)cli_fd)->fd;
    char buf[100];
    int r = 0;
    while (1) {
        r = lthread_read(fd, buf, 100, 10);
        if (r > 0)
            printf("read from server: %.*s\n", r, buf);
    }
}

void
socket_client_server_writer(void *cli_fd)
{
    char buf[100] = "Hello from client!";
    int fd = ((struct conn *)cli_fd)->fd;
    printf("fd is %d\n", fd);
    while (1) {
        lthread_write(fd, buf, strlen(buf));
        lthread_sleep(1000);
    }
}

void
client(void *arg)
{
    int s, t, len;
    int i = 10;
    struct sockaddr_un remote;
    char str[100];
    lthread_t *lt;
    remote.sun_family = PF_UNIX;
    strcpy(remote.sun_path, "/tmp/lthread.sock");
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if ((s = lthread_socket(PF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    if (lthread_connect(s, (struct sockaddr *)&remote, len, 0) == -1) {
        perror("connect");
        exit(1);
    }
    struct conn *c = calloc(1, sizeof(struct conn));
    c->fd = s;
    lthread_create(&lt, socket_client_server_reader, c);
    lthread_create(&lt, socket_client_server_writer, c);
}

void
socket_server_client_reader(void *cli_fd)
{
    int fd = ((struct conn *)cli_fd)->fd;
    char buf[100];
    int r = 0;
    printf("socket_server_client_reader started\n");
    while (1) {
        r = lthread_read(fd, buf, 100, 0);
        printf("read from client: %.*s\n", r, buf);
    }
}

void
socket_server_client_writer(void *cli_fd)
{
    int fd = ((struct conn *)cli_fd)->fd;
    char buf[100] = "Hello from server!";
    printf("socket_server_client_writer started\n");
    while (1) {
        lthread_write(fd, buf, strlen(buf));
        lthread_sleep(1000);
    }
}

void
server(void *arg)
{
    int fd = 0;
    int cli_fd = 0;
    int len = 0;
    lthread_t *lt;
    struct sockaddr_un local, remote;
    fd = lthread_socket(PF_UNIX, SOCK_STREAM, 0);

    local.sun_family = PF_UNIX;
    strcpy(local.sun_path, "/tmp/lthread.sock");
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);

    bind(fd, (struct sockaddr *)&local, len);
    listen(fd, 100);

    while (1) {
        len = sizeof(struct sockaddr_un);
        cli_fd = lthread_accept(fd, (struct sockaddr *)&remote, &len);
        struct conn *c = calloc(1, sizeof(struct conn));
        c->fd = cli_fd;
	    lthread_create(&lt, socket_server_client_reader, c);
	    lthread_create(&lt, socket_server_client_writer, c);
    }
}

int
main(int argc, char **argv)
{
	lthread_t *lt = NULL;

	lthread_create(&lt, server, NULL);
	lthread_create(&lt, client, NULL);
	lthread_run();

	return 0;
}
