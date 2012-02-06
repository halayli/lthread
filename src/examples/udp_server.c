#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lthread.h>

void
udp_server(lthread_t *lt, void *args)
{
    struct sockaddr_in listener;
    struct sockaddr_in client;
    socklen_t listener_len = sizeof(listener);
    int s;
    int ret;
    char buf[64];
    lthread_detach();

    if ((s=lthread_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
        perror("error");
        return;
    }

    memset((char *) &listener, 0, sizeof(listener));
    listener.sin_family = AF_INET;
    listener.sin_port = htons(5556);
    listener.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr *)&listener, sizeof(listener)) == -1) {
        perror("Cannot bind");
        return;
    }

    while (1) {
        ret = lthread_recvfrom(s, buf, 64, 0, (struct sockaddr *)&client, &listener_len, 1000);
        printf("ret returned %d: %s\n", ret, buf);
    }

   close(s);
}
 
int
main(void)
{
    lthread_t *lt;
    lthread_create(&lt, udp_server, NULL);
    lthread_run();

    return 0;
 }
