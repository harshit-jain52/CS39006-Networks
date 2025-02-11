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

#define max(x,y) ((x) > (y) ? (x) : (y))
#define MINCLIENTS 2
#define MAXCLIENTS 5
#define MAXBUFLEN 500
#define PORT 8008

int main(){
    int numclient=0, N=1;
    struct sockaddr_in addr, client_addr;
    socklen_t addrlen;
    int serversocketfd, yes;
    int clientsocketfd[MAXCLIENTS];
    fd_set master, rfds;
    int maxfd;
    bool game_started = false;
    int numbers[MAXCLIENTS];
    bool is_received[MAXCLIENTS], is_closed[MAXCLIENTS];
    char buf[MAXBUFLEN];
    struct timeval tv;

    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(PORT);
    addr.sin_family = AF_INET;
    inet_aton("127.0.0.1",&addr.sin_addr);

    serversocketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serversocketfd == -1) {
        perror("socket");
        exit(1);
    }

    yes = 1;
    if (setsockopt(serversocketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        close(serversocketfd);
        exit(1);
    }

    if (bind(serversocketfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind");
        close(serversocketfd);
        exit(1);
    }

    listen(serversocketfd,MAXCLIENTS);

	FD_ZERO(&master);
    FD_SET(serversocketfd,&master);
    maxfd = serversocketfd;
    
    for(;;){
        rfds = master;
        tv.tv_sec=1;
        tv.tv_usec=0;
        select(maxfd+1, &rfds, NULL, NULL, &tv);

        if(FD_ISSET(serversocketfd,&rfds)){
            addrlen = sizeof(client_addr);
            int clfd = accept(serversocketfd, (struct sockaddr *) &client_addr, &addrlen);
            if(clfd==-1){
                perror("accept");
            }
            else{
                if(numclient+1 > MAXCLIENTS){
                    printf("Server: Maximum %d clients allowed\n", MAXCLIENTS);
                }
                else{
                    numclient++;
                    FD_SET(clfd,&master);
                    maxfd = max(maxfd, clfd);
                    clientsocketfd[numclient-1]=clfd;
                    is_received[numclient-1]=false;
                    is_closed[numclient-1]=false;
                    printf("Server: Received a new connection from client %s:%d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
                    if(game_started){
                        sprintf(buf,"Server: Send your number for Round %d", N);
                        send(clientsocketfd[numclient-1],buf,strlen(buf)+1,0);
                    }
                }
            }
        }
        else{
            for(int i=0;i<numclient;i++){
                if(FD_ISSET(clientsocketfd[i],&rfds)){
                    int numbytes = recv(clientsocketfd[i],buf,MAXBUFLEN,0);
                    if(numbytes<=0){
                        if(numbytes<0) perror("recv");
                        else{
                            is_closed[i]=true;
                            close(clientsocketfd[i]);
                        }
                    }
                    else{
                        buf[numbytes]='\0';
                        int num = atoi(buf);
                        addrlen = sizeof(client_addr);
                        getpeername(clientsocketfd[i], (struct sockaddr *) &client_addr, &addrlen);
                        if(!game_started){
                            printf("Server: Insufficient clients, \"%s\" from client %s:%d dropped\n",buf,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
                        }
                        else{
                            if(is_received[i]){
                                sprintf(buf,"Server: Duplicate messages for Round %d are not allowed. Please wait for the results for Round %d and Call for the number for Round %d",N,N,N+1);
                                send(clientsocketfd[i],buf,strlen(buf)+1,0);
                            }
                            else{
                                is_received[i]=true;
                                numbers[i]=num;
                            }
                        }
                    }
                    break;
                }
            }
        }

        if(game_started){
            bool flag = true;
            for(int i=0;i<numclient;i++){
                if(!is_received[i] && !is_closed[i]) flag=false;
            }
            if(flag){
                int maxnum = INT_MIN;
                for(int i=0;i<numclient;i++){
                    maxnum = max(maxnum,numbers[i]);
                    is_received[i] = false;
                }

                for(int i=0;i<numclient;i++){
                    if(numbers[i]==maxnum){
                        addrlen = sizeof(client_addr);
                        getpeername(clientsocketfd[i], (struct sockaddr *) &client_addr, &addrlen);
                        sprintf(buf, "Server: Maximum Number Received in Round %d is: %d. The number has been received from the client %s:%d",N,maxnum,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));             
                        break;
                    }
                }

                for(int i=0;i<numclient;i++){
                    send(clientsocketfd[i],buf,strlen(buf)+1,0);
                }
                sleep(1);
                sprintf(buf,"Enter the number for Round %d:",N+1);
                for(int i=0;i<numclient;i++){
                    send(clientsocketfd[i],buf,strlen(buf)+1,0);
                }
                N++;
            }
        }
        else{
            if(numclient>=MINCLIENTS){
                game_started=true;
                sprintf(buf,"Enter the number for Round 1");
                for(int i=0;i<numclient;i++){
                    send(clientsocketfd[i],buf,strlen(buf)+1,0);
                }
            }
        }
    }
}