/*
===================================== 
Assignment 3 Submission - retrieveencfileclient.c
Name: Harshit Jain
Roll number: 22CS10030
Link of the pcap files: https://drive.google.com/drive/folders/1Cyq3CTAa_JNXHYaJ23GvDwBiSrt4IXt3?usp=sharing
===================================== 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>

#define SERVERPORT 8008
#define MAXBUFLEN 100
#define KEYLEN 26
#define CHUNKSIZE 40

bool validate_key(char key[]){
    if(strlen(key) != 26){
        printf("Key must consist of exactly 26 characters\n");
        return false;
    }

    int ct[26];
    memset(ct,0,sizeof(ct));

    for(int i=0; key[i]!='\0'; i++){
        if(!isalpha(key[i])){
            printf("Key must consist of alphabetical characters only\n");
            return false;
        }
        key[i] = toupper(key[i]);
        ct[key[i]-'A']++;
    }

    for(int i=0; i<26; i++){
        if(ct[i]!=1){
            printf("Key must have distinct characters\n");
            return false;
        }
    }
    return true;
}

void send_file_by_chunks(int sockfd, FILE* fp){
    char chunk[CHUNKSIZE];
    char eof_marker = '\0';

    size_t bytes_read;
    while((bytes_read = fread(chunk, 1, CHUNKSIZE, fp)) > 0){
        if(send(sockfd, chunk, bytes_read, 0) < 0){
            perror("server: send");
            fclose(fp);
            close(sockfd);
            exit(1);
        }
    }
    send(sockfd, &eof_marker, 1, 0);
}

int main(){
    char buf[MAXBUFLEN];

	struct sockaddr_in servaddr;

    // TCP Socket
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("client: socket");
        exit(1);
    }

    // Server Address
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVERPORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    // Connect to server
    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        perror("client: connect");
        exit(1);
    }
    printf("client: connection established with server\n");

    while(1){
        char filename[100], encfile[105], key[100];
        bool gotfile = false, gotkey = false;
        FILE* fp = NULL;

        // Take filename as input
        do{
            printf("Enter filename: ");
            scanf("%s", filename);

            fp = fopen(filename,"r");
            if(fp == NULL) printf("NOTFOUND %s\n", filename);
            else gotfile = true;
        } while(!gotfile);

        // Take KEY as input
        do{
            printf("Enter key: ");
            scanf("%s", key);
            gotkey = validate_key(key);
        } while(!gotkey);

        // Send Key to server
        send(sockfd, key, strlen(key), 0);

        // Send the file to server
        send_file_by_chunks(sockfd, fp);
        fclose(fp);

        char eof_marker = '\0';

        sprintf(encfile,"%s.enc",filename);
        fp = fopen(encfile,"w");
        while(1){
            // Receive bytes of encrypted file from server
            int numbytes = recv(sockfd, buf, MAXBUFLEN, 0);
            if(numbytes < 0){
                close(sockfd);
                perror("client: recv");
                exit(1);
            }
            else if(numbytes == 0){
                break;
            }

            // Check for EOF
            char* eof = memchr(buf, eof_marker, numbytes);
            numbytes = eof == NULL ? numbytes : eof - buf;
            fwrite(buf, 1, numbytes, fp);
            if(eof != NULL) break;
        }
        fclose(fp);

        printf("File encrypted with substitution cipher using key %s\n",key);
        printf("Plaintext: %s\nCiphertext: %s\n", filename, encfile);

        printf("\nEnter \"No\" (case-sensitive) to exit, anything else to encrypt another file: ");
        scanf("%s", buf);
        if(strcmp(buf,"No") == 0) break;
        send(sockfd, &eof_marker, 1, 0); // Send an indicating byte to server
    }

    close(sockfd);
}