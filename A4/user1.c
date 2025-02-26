#include <ksocket.h>
char buf[MSGSIZE];
const char *eof_marker = "~";

int main(int argc, char *argv[])
{
    if(argc != 5){
        printf("Usage: %s <src_ip> <src_port> <dest_ip> <dest_port>\n", argv[0]);
        return -1;
    }
    const char *src_ip = argv[1];
    int src_port = atoi(argv[2]);
    const char *dest_ip = argv[3];
    int dest_port = atoi(argv[4]);

    ksockfd_t sockfd = k_socket(AF_INET, SOCK_KTP, 0);
    if (sockfd < 0)
    {
        perror("user1: k_socket");
        return -1;
    }

    if ((k_bind(sockfd, src_ip, src_port, dest_ip, dest_port)) < 0)
    {
        perror("user1: k_bind");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(dest_port);
    addr.sin_addr.s_addr = inet_addr(dest_ip);

    // FILE *fp = fopen("lorem_10KB.txt", "r");
    FILE *fp = fopen("lorem_100KB.txt", "r");
    if (fp == NULL)
    {
        perror("user1: fopen");
        return -1;
    }
    sleep(2);
    while (1)
    {
        size_t bytesRead = fread(buf, 1, MSGSIZE, fp);
        if (bytesRead == 0)
        {
            if (feof(fp))
            {
                printf("user1: End of file reached.\n");
                memcpy(buf, eof_marker, 1);
                bytesRead = 1;
            }
            else
            {
                perror("user1: fread");
                fclose(fp);
                return -1;
            }
        }

    again:
        int numbytes = k_sendto(sockfd, buf, bytesRead, 0, (struct sockaddr *)&addr, sizeof(addr));
        if (numbytes < 0)
        {
            perror("user1: k_send");
            if (errno == ENOSPACE || errno == ENOTBOUND)
            {
                sleep(1);
                goto again;
            }
            fclose(fp);
            k_close(sockfd);
            return -1;
        }
        printf("user1: Sent %d bytes.\n", numbytes);
        if(memcmp(buf, eof_marker, 1) == 0)
        {
            printf("user1: EOF marker sent.\n");
            break;
        }
    }

    fclose(fp);
    sleep(100);
    k_close(sockfd);
    return 0;
}