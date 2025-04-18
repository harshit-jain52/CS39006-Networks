/*
===================================== 
Assignment 2 Submission - wordclient.c
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
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h> 
#include <string.h>

#define SERVERPORT 8008
#define MAXLINE 1000 
#define MAXBUFLEN 500

int main(int argc, const char* argv[]){
    if(argc<2){
        printf("Usage: %s [filename]",argv[0]);
        exit(1);
    }

    char buffer[MAXBUFLEN]; // Buffer to receive message in
    char req[MAXLINE];      // Message to send

    // Special messages
    char notfounderr[MAXLINE];
    sprintf(notfounderr, "NOTFOUND %s", argv[1]);
    char *hellomsg = "HELLO";
    char *finishmsg = "FINISH";

    // Server Address
    struct sockaddr_in servaddr; 
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(SERVERPORT); 
    servaddr.sin_family = AF_INET;
    
    // UDP Socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("client: socket");
        exit(1);
    }

    // Set timeout (of 5 seconds) for recvfrom
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv, sizeof(tv)) < 0){
        perror("client: setsockopt");
        exit(1);
    }

    // Send filename to server
    if(sendto(sockfd, argv[1], strlen(argv[1]), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        close(sockfd);
        perror("client: sendto");
        exit(1);
    }

    int ct = 0;
    FILE* fp;

    while(++ct){        
        fflush(stdout);

        // Receive message from server
        int numbytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL); 
        if(numbytes < 0){
            close(sockfd);
            perror("client: recvfrom");
            exit(1);
        }
        buffer[numbytes] = '\0';
        printf("Response from Server: %s\n",buffer);

        if(!strcmp(buffer,notfounderr)){
            printf("FILE NOT FOUND\n");
            break;
        }

        // On HELLO, Create a text file to store the responses
        if(!strcmp(buffer, hellomsg)){
            char fname[100];
            sprintf(fname,"response_%s",argv[1]);
            fp = fopen(fname, "w");
        }

        if(fp == NULL){
            perror("client: fopen");
            exit(1);
        }
        
        // Write the response to the file
        fprintf(fp, "%s\n", buffer);
        
        // On FINISH, close the file and exit
        if(!strcmp(buffer, finishmsg)){
            fclose(fp);
            break;
        }

        // Send WORD request to server
        sprintf(req, "WORD %d", ct); 
        if(sendto(sockfd, req, strlen(req), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
            close(sockfd);
            perror("client: sendto");
            exit(1);
        }
    }
    
    close(sockfd);
    return 0;
}