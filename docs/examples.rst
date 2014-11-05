Examples
========

Webserver
---------


`gcc -I/usr/local/include -llthread test.c -o test`

.. code-block:: C

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <lthread.h>

    struct cli_info {
        /* other stuff if needed*/
        struct sockaddr_in peer_addr;
        int fd;
    };

    typedef struct cli_info cli_info_t;

    char *reply = "HTTP/1.0 200 OK\r\nContent-length: 11\r\n\r\nHello World";

    unsigned int
    fibonacci(unsigned int n)
    {
        if (n == 0)
            return 0;
         if (n == 1)
            return 1;

         return fibonacci(n - 1) + fibonacci(n - 2);
    }

    void
    http_serv(lthread_t *lt, void *arg)
    {
        cli_info_t *cli_info = arg;
        char *buf = NULL;
        int ret = 0;
        char ipstr[INET6_ADDRSTRLEN];
        lthread_detach();

        inet_ntop(AF_INET, &cli_info->peer_addr.sin_addr, ipstr, INET_ADDRSTRLEN);
        printf("Accepted connection on IP %s\n", ipstr);

        if ((buf = malloc(1024)) == NULL)
            return;

        /* read data from client or timeout in 5 secs */
        ret = lthread_recv(cli_info->fd, buf, 1024, 0, 5000);

        /* did we timeout before the user has sent us anything? */
        if (ret == -2) {
            lthread_close(cli_info->fd);
            free(buf);
            free(arg);
            return;
        }

        /*
         * Run an expensive computation without blocking other lthreads.
         * lthread_compute_begin() will yield http_serv coroutine and resumes
         * it in a compute scheduler that runs in a pthread. If a compute scheduler
         * is already available and free it will be used otherwise a compute scheduler
         * is created and launched in a new pthread. After the compute scheduler
         * resumes the lthread it will wait 60 seconds for a new job and dies after 60
         * of inactivity.
         */
        lthread_compute_begin();
            /* make an expensive call without blocking other coroutines */
            ret = fibonacci(35);
        lthread_compute_end();

        /* reply back to user */
        lthread_send(cli_info->fd, reply, strlen(reply), 0);
        lthread_close(cli_info->fd);
        free(buf);
        free(arg);
    }

    void
    listener(void *arg)
    {
        int cli_fd = 0;
        int lsn_fd = 0;
        int opt = 1;
        int ret = 0;
        struct sockaddr_in peer_addr = {};
        struct   sockaddr_in sin = {};
        socklen_t addrlen = sizeof(peer_addr);
        lthread_t *cli_lt = NULL;
        cli_info_t *cli_info = NULL;
        char ipstr[INET6_ADDRSTRLEN];
        lthread_detach();

        DEFINE_LTHREAD;

        /* create listening socket */
        lsn_fd = lthread_socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (lsn_fd == -1)
            return;

        if (setsockopt(lsn_fd, SOL_SOCKET, SO_REUSEADDR, &opt,sizeof(int)) == -1)
            perror("failed to set SOREUSEADDR on socket");

        sin.sin_family = PF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(3128);

        /* bind to the listening port */
        ret = bind(lsn_fd, (struct sockaddr *)&sin, sizeof(sin));
        if (ret == -1) {
            perror("Failed to bind on port 3128");
            return;
        }

        printf("Starting listener on 3128\n");

        listen(lsn_fd, 128);

        while (1) {
            /* block until a new connection arrives */
            cli_fd = lthread_accept(lsn_fd, (struct sockaddr*)&peer_addr, &addrlen);
            if (cli_fd == -1) {
                perror("Failed to accept connection");
                return;
            }

            if ((cli_info = malloc(sizeof(cli_info_t))) == NULL) {
                close(cli_fd);
                continue;
            }
            cli_info->peer_addr = peer_addr;
            cli_info->fd = cli_fd;
            /* launch a new lthread that takes care of this client */
            ret = lthread_create(&cli_lt, http_serv, cli_info);
        }
    }

    int
    main(int argc, char **argv)
    {
        lthread_t *lt = NULL;

        lthread_create(&lt, listener, NULL);
        lthread_run();

        return 0;
    }

::
