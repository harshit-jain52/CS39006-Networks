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

    if ((k_bind(sockfd, IP, PORT1, IP, PORT2)) < 0)
    {
        perror("user1: k_bind");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT2);
    addr.sin_addr.s_addr = inet_addr(IP);
    sleep(2);
    for (int i = 0; i < 10; i++)
    {
        memset(buf, 'A' + i, MSGSIZE);
        int numbytes = k_sendto(sockfd, buf, MSGSIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
        if (numbytes < 0)
        {
            perror("user1: k_send");
            return -1;
        }

        printf("user1: Sent %d bytes.\n", numbytes);
    }
    pause();
}