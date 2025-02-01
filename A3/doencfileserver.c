/*
===================================== 
Assignment 3 Submission - doencfileserver.c
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
#include <ctype.h>

#define PORT 8008
#define MAXBUFLEN 100
#define KEYLEN 26
#define MAXCLIENTS 5
#define CHUNKSIZE 30

int min(int x, int y){
    return x < y ? x : y;
}

char substitute_char(char c, char key[]){
    if(!isalpha(c)) return c;
    if(islower(c)) return tolower(key[c-'a']);
    return key[c-'A'];
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

    socklen_t addr_len;
    struct sockaddr_in servaddr, cliaddr;

    // TCP Socket
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("server: socket");
        exit(1);
    }

    // Server Address
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;
    memset(servaddr.sin_zero, '\0', sizeof(servaddr.sin_zero));

    // Bind server address to socket
    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        perror("server: bind");
        exit(1);
    }

    // Allow to reuse the port when rerun
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("server: setsockopt");
        exit(1);
    }

    // Listen for incoming connections
    if(listen(sockfd, MAXCLIENTS) < 0){
        perror("server: listen");
        exit(1);
    }

    while(1){
        // Accept an incoming connection
        addr_len = sizeof(cliaddr);
        int newsockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &addr_len);
        if(newsockfd < 0){
            perror("server: accept");
            exit(1);
        }
        printf("server: connection extablished with client at port %d\n", ntohs(cliaddr.sin_port));

        // Fork for concurrency
        pid_t pid = fork();
        if(pid < 0){
            perror("server: fork");
            exit(1);
        }

        if(pid == 0){
            close(sockfd); // Close old socket

            // Get client's IPv4 address
            char client_ip4[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip4, INET_ADDRSTRLEN);

            char filename[INET_ADDRSTRLEN+15], encfile[INET_ADDRSTRLEN+20];
            sprintf(filename,"%s.%d.txt",client_ip4,ntohs(cliaddr.sin_port));
            sprintf(encfile,"%s.enc",filename);
            
            while(1){
                FILE* fp = fopen(filename, "w");
                if(fp == NULL){
                    perror("server: fopen");
                    exit(1);
                }

                char key[KEYLEN];
                int keylen = 0;
                char eof_marker = '\0';

                while(1){
                    // Receive bytes of plain file from client
                    int numbytes = recv(newsockfd, buf, MAXBUFLEN, 0);
                    if(numbytes < 0){
                        close(newsockfd);
                        perror("server: recv");
                        exit(1);
                    }
                    else if(numbytes == 0){
                        goto close_conn;
                    }

                    // Check for EOF
                    char* eof = memchr(buf, eof_marker, numbytes);
                    numbytes = eof == NULL ? numbytes : eof - buf;

                    // Check if the key has been read completely
                    int key_left = KEYLEN - keylen;
                    if(key_left > 0){
                        int key_recv = min(numbytes, key_left);
                        memcpy(key+keylen, buf, key_recv);
                        keylen += key_recv;

                        if(numbytes > key_recv){
                            fwrite(buf+key_recv, 1, numbytes-key_recv, fp);
                        }
                    }
                    else{
                        fwrite(buf, 1, numbytes, fp);
                    }
                    if(eof != NULL) break;
                }
                fclose(fp);

                // Read plaintext (char-by-char) and encrypt it to write ciphertext
                FILE* plain = fopen(filename,"r");
                FILE* cipher = fopen(encfile,"w");
                char ch;
                while((ch = fgetc(plain)) != EOF) fputc(substitute_char(ch,key),cipher);
                fclose(plain);
                fclose(cipher);

                // Send encrypted file to client
                fp = fopen(encfile,"r");
                send_file_by_chunks(newsockfd, fp);
                fclose(fp);

                // Wait for client to close connection or send another file
                int numbytes = recv(newsockfd, buf, 1, 0);
                if(numbytes < 0){
                    close(newsockfd);
                    perror("server: recv");
                    exit(1);
                }
                else if(numbytes == 0){
                    goto close_conn;
                }
            }
            close_conn: // Close Connection
            printf("server: connection closed by client at port %d\n", ntohs(cliaddr.sin_port));
            close(newsockfd);
            exit(0);
        }
        else{
            close(newsockfd);
        }
    }
}