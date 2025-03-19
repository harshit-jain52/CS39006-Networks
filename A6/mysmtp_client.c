#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXCLIENTS 5
#define MAXBUFLEN 300
#define MAXURLLEN 100
#define MAXDATALEN 200

const char SEPERATOR = '.';

// Commands

#define DATA "DATA\n"
#define GOODBYE "200 Goodbye\n"

int main(int argc, char* argv[]){
    if(argc != 3){
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("client: socket");
        exit(1);
    }

    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        perror("client: connect");
        exit(1);
    }

    printf("Connected to My_SMTP server at %s:%d\n", argv[1], atoi(argv[2]));

    while(1){
        fflush(stdout);
        printf("> ");
        char buf[MAXBUFLEN];
        fgets(buf, sizeof(buf), stdin);
        if(!memcmp(buf, DATA, strlen(DATA))){
            int sz = strlen(DATA);
            printf("Enter your message (end with a single dot '.'):\n");
            while(1){
                char line[MAXDATALEN];
                memset(line, 0, sizeof(line));
                fgets(line, sizeof(line), stdin);
                if(!strcmp(line, ".\n")){
                    break;
                }
                strcpy(buf+sz, line);
                sz += strlen(line);
            }
        }
        buf[strlen(buf)-1] = '\0';
        send(sockfd, buf, strlen(buf)+1, 0);
        int numbytes = recv(sockfd, buf, MAXBUFLEN, 0);
        if(numbytes < 0){
            perror("client: recv");
            exit(1);
        }
        else if(numbytes == 0){
            printf("client: connection closed by server\n");
            break;
        }
        
        buf[numbytes] = '\0';
        printf("%s", buf);
        if(strcmp(buf, GOODBYE) == 0){
            break;
        }
    }

    close(sockfd);
    return 0;
}