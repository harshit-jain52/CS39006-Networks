#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#define MAXBUFLEN 500
#define SERVERPORT 8008

int main(){
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen;
    int clientsocketfd, yes;
    fd_set master, rfds;
    int maxfd;
    struct timeval tv;
    char buf[MAXBUFLEN];

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_port = htons(SERVERPORT);
    server_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1",&server_addr.sin_addr);

    clientsocketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientsocketfd == -1) {
        perror("socket");
        exit(1);
    }

    connect(clientsocketfd, (struct sockaddr *) &server_addr, sizeof(server_addr));

	FD_ZERO(&master);
    FD_SET(STDIN_FILENO,&master);
    FD_SET(clientsocketfd,&master);
    maxfd = clientsocketfd;
    
    for(;;){
        rfds = master;
        tv.tv_sec=1;
        tv.tv_usec=0;
        select(maxfd+1, &rfds, NULL, NULL, &tv);

        if(FD_ISSET(STDIN_FILENO,&rfds)){
            int n = read(STDIN_FILENO,buf,MAXBUFLEN);
            buf[n-1]='\0';
            if(atoi(buf) > 0){
                send(clientsocketfd,buf,strlen(buf)+1,0);
                addrlen = sizeof(client_addr);
                getsockname(clientsocketfd, (struct sockaddr *) &client_addr, &addrlen);
                printf("Client: %s:%d Number %s sent to server\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port),buf);
            }
        }
        else if(FD_ISSET(clientsocketfd,&rfds)){
            int numbytes = recv(clientsocketfd,buf,MAXBUFLEN,0);
            if(numbytes<0){
                perror("recv");
            }
            else if(numbytes==0){
                close(clientsocketfd);
                break;
            }
            else{
                buf[numbytes]='\0';
                printf("Client: Received the following message text from the server: %s\n", buf);
            }
        }
    }
}