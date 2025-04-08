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
const char* ACK = "Received";

int create_udp_server_socket(int port){
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_socket < 0){
        perror("socket");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind");
        close(udp_socket);
        exit(1);
    }

    int flags = fcntl(udp_socket, F_GETFL, 0);
    if(flags<0){
        perror("fcntl F_GETFL");
        close(udp_socket);
        exit(1);
    }
    if(fcntl(udp_socket, F_SETFL, flags | O_NONBLOCK) < 0){
        perror("fcntl F_SETFL");
        close(udp_socket);
        exit(1);
    }
    return udp_socket;
}

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
    int sockfd = create_udp_server_socket(PORT);
    set_udp_socket_options(sockfd);

    char buff[MAXBUFLEN];

    fd_set master, rfds;
    FD_ZERO(&master);
    FD_SET(sockfd, &master);
    int maxfd = sockfd;

    FILE* fp = fopen("udp_server.log","w");
    while(1){
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        rfds = master;

        select(maxfd+1, &rfds, NULL, NULL, &tv);
        if(FD_ISSET(sockfd, &rfds)){
            struct sockaddr_in cli_addr;
            socklen_t addrlen = sizeof(cli_addr);

            int numbytes = recvfrom(sockfd, buff, MAXBUFLEN, 0, (struct sockaddr*)&cli_addr, &addrlen);
            if(numbytes<=0){
                if(numbytes<0){
                    perror("recvfrom");
                }
                continue;
            }
            buff[numbytes-1]='\0';
            const time_t timestamp = time(NULL);
            struct tm *tm = localtime(&timestamp);
            char timebuf[64];
            char totalbuff[5096];
            strftime(timebuf, 64, "%Y-%m-%d %H:%M:%S", tm);
            sprintf(totalbuff,"[%s] From %s:%d - %s", timebuf, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), buff);
            printf("%s\n", totalbuff);
            fprintf(fp,"%s\n",totalbuff);
            fflush(fp);
            sprintf(buff,"%s",ACK);
            sendto(sockfd, buff, strlen(buff), 0, (struct sockaddr*)&cli_addr, sizeof(cli_addr));
        }
    } 
    
    close(sockfd);
    fclose(fp);
}