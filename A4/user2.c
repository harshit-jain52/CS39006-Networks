#include "ksocket.h"
#define PORT1 5050
#define PORT2 5051
const char *IP = "127.0.0.1";
char buf[MSGSIZE];
const char *eof_marker = "~";

int main()
{
    ksockfd_t sockfd = k_socket(AF_INET, SOCK_KTP, 0);
    if (sockfd < 0)
    {
        perror("user2: k_socket");
        return -1;
    }

    if ((k_bind(sockfd, IP, PORT2, IP, PORT1)) < 0)
    {
        perror("user2: k_bind");
        return -1;
    }

    // FILE *fp = fopen("received_10KB.txt", "w");
    FILE *fp = fopen("received_100KB.txt", "w");
    if (fp == NULL)
    {
        perror("user2: fopen");
        return -1;
    }
    while (1)
    {
        sleep(2);
        int n = k_recvfrom(sockfd, buf, MSGSIZE, 0, NULL, 0);
        if (n < 0)
        {
            perror("user2: k_recv");
            sleep(1);
            continue;
        }

        printf("user2: received %d bytes\n", n);

        if (memcmp(buf, eof_marker, 1) == 0)
        {
            printf("user2: EOF marker received\n");
            break;
        }

        fwrite(buf, 1, n, fp);
        fflush(fp);
    }

    fclose(fp);
    k_close(sockfd);
    return 0;
}