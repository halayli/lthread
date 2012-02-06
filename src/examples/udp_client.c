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
udp_client(lthread_t *lt, void *args)
{
    struct sockaddr_in dest;
    socklen_t dest_len = sizeof(dest);
    int s;
    int ret;
    char buf[64] = "hello world!";
    lthread_detach();

    if ((s=lthread_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
        perror("error");
        return;
    }

    memset((char *) &dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(5556);
    if (inet_aton("127.0.0.1", &dest.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    while (1) {
        ret = lthread_sendto(s, buf, 64, 0, (struct sockaddr *)&dest, dest_len);
        printf("ret returned %d\n", ret);
        lthread_sleep(1000);
    }

   close(s);
}
 
int
main(void)
{
    lthread_t *lt;
    lthread_create(&lt, udp_client, NULL);
    lthread_run();

    return 0;
 }
