#include <stdio.h> 
#include <strings.h> 
#include <sys/types.h> 
#include <arpa/inet.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define PORT 8008
#define MAXLINE 1000 
#define MAXBUFLEN 500

int main(){    
    char buffer[MAXBUFLEN]; // Buffer to receive message in
    char res[MAXLINE];      // Message to send

    socklen_t addr_len;
    struct sockaddr_in servaddr, cliaddr; 
  
    // Server Address
    bzero(&servaddr, sizeof(servaddr));             // Clear servaddr struct
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);   // local IP
    servaddr.sin_port = htons(PORT);
    servaddr.sin_family = AF_INET;                  // IPv4

    // Create a UDP Socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1){
        perror("server: socket");
        exit(1);
    }

    // Bind server address to socket descriptor 
    int status = bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if(status == -1){
        close(sockfd);
        perror("server: bind");
        exit(1);
    }
    
    printf("\nServer Running at port %d\n\n", PORT);
    
    FILE* fp = NULL;

    while(1){
        // Receive message from client and store client address
        addr_len = sizeof(cliaddr);
        int numbytes = recvfrom(sockfd, buffer, MAXBUFLEN-1, 0, (struct sockaddr*)&cliaddr, &addr_len);
        if(numbytes==-1){
            close(sockfd);
            perror("server: recvfrom");
            exit(1);
        }
        buffer[numbytes] = '\0';
        printf("Request from Client at port %d: %s\n",ntohs(cliaddr.sin_port),buffer);
        
        if(!fp){
            // Open file (filename received in the request)
            fp = fopen(buffer, "r");

            // File Not Found
            if(fp == NULL){
                printf("\nFile not found\n");
                sprintf(res, "NOTFOUND %s", buffer);
                if(sendto(sockfd, res, strlen(res), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) == -1){
                    close(sockfd);
                    perror("server: sendto");
                    exit(1);
                }
                break;
            }
        }

        // Read one word from the file and send it to client
        fscanf(fp, "%s", res);
        if(sendto(sockfd, res, strlen(res), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) == -1){
            close(sockfd);
            perror("server: sendto");
            exit(1);
        }

        // Close the file and exit when EOF reached
        if(feof(fp)){
            fclose(fp);
            break;
        }
    }

    close(sockfd);
    return 0;
}