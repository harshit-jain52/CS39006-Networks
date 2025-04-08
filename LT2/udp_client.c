#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#define PORT 9090
#define MAXBUFLEN 4096

void set_udp_socket_options(int sockfd){
    int yes = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt SO_REUSEADDR");
        close(sockfd);
        exit(1);
    }

    int sz = MAXBUFLEN;
    if(setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sz, sizeof(sz)) == -1) {
        perror("setsockopt SO_SNDBUF");
        close(sockfd);
        exit(1);
    }

    printf("Send Buffer Size passed to setsockopt(): %d\n", sz);
    socklen_t len;
    int _ = getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sz, &len);
    if(_ == -1){
        perror("getsockopt SO_SNDBUF");
        close(sockfd);
        exit(1);
    }
    printf("Send Buffer Size returned from getsockopt(): %d\n", sz);

    /*
    > man 7 socket
    SO_SNDBUF
        Sets  or gets the maximum socket send buffer in bytes.  The ker‐
        nel doubles this value (to allow space for bookkeeping overhead)
        when  it  is  set using setsockopt(2), and this doubled value is
        returned by getsockopt(2).
    */
}

int main(){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    char buff[MAXBUFLEN];
    if(sockfd < 0){
        perror("socket");
        exit(1);
    }

    set_udp_socket_options(sockfd);
    struct sockaddr_in serv_addr;
    socklen_t addr_len = sizeof(serv_addr);
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while(1){
        printf("Enter a message:\n");
        int numbytes = read(0, buff, MAXBUFLEN);
        numbytes = sendto(sockfd, buff, numbytes, 0, (struct sockaddr*)&serv_addr, addr_len);
        numbytes = recvfrom(sockfd, buff, MAXBUFLEN, 0, (struct sockaddr*)&serv_addr, &addr_len);
        buff[numbytes]='\0';
        printf("Server: %s\n", buff);
    }
}