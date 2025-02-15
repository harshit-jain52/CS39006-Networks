#include "ksocket.h"
#define PORT1 5050
#define PORT2 5051
const char *IP = "127.0.0.1";
char buf[MSGSIZE];

int main()
{
    ksockfd_t sockfd = k_socket(AF_INET, SOCK_KTP, 0);
    if (sockfd < 0)
    {
        perror("user1: k_socket");
        return -1;
    }

    if ((k_bind(sockfd, IP, PORT2, IP, PORT1)) < 0)
    {
        perror("user1: k_bind");
        return -1;
    }

    while (1)
    {   
        sleep(3);
        int n = k_recvfrom(sockfd, buf, MSGSIZE, 0, NULL, 0);
        if (n < 0)
        {
            perror("user2: k_recv");
        }
        printf("user2: received %s\n", buf);
    }
}