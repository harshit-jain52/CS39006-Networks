/*
===================================== 
Assignment 2 Submission - wordserver.c
Name: Harshit Jain
Roll number: 22CS10030
Link of the pcap file: https://drive.google.com/file/d/1czO9qor2WBC5euc79yZ8aToR0SkmtKCR/view?usp=sharing
===================================== 
*/

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8008
#define MAXLINE 1000 
#define MAXBUFLEN 500

int main(){    
    char buffer[MAXBUFLEN]; // Buffer to receive message in
    char res[MAXLINE];      // Message to send
    char *finishmsg = "FINISH";

    socklen_t addr_len;
    struct sockaddr_in servaddr, cliaddr; 
  
    // Server Address
    bzero(&servaddr, sizeof(servaddr));             // Clear servaddr struct
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);   // local IP
    servaddr.sin_port = htons(PORT);
    servaddr.sin_family = AF_INET;                  // IPv4

    // Create a UDP Socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("server: socket");
        exit(1);
    }

    // Bind server address to socket descriptor 
    int status = bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if(status < 0){
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
        if(numbytes < 0){
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
                if(sendto(sockfd, res, strlen(res), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) < 0){
                    close(sockfd);
                    perror("server: sendto");
                    exit(1);
                }
                break;
            }
        }

        // Read one word from the file and send it to client
        fscanf(fp, "%s", res);
        if(sendto(sockfd, res, strlen(res), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) < 0){
            close(sockfd);
            perror("server: sendto");
            exit(1);
        }

        // Close the file and exit when FINISH is encountered
        if(!strcmp(res, finishmsg)){
            fclose(fp);
            break;
        }
    }

    close(sockfd);
    return 0;
}