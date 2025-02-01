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

int main(){
    char buf[MAXBUFLEN];
    char chunk[CHUNKSIZE];

    socklen_t addr_len;
    struct sockaddr_in servaddr, cliaddr;

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("server: socket");
        exit(1);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    memset(servaddr.sin_zero, '\0', sizeof(servaddr.sin_zero));

    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        perror("server: bind");
        exit(1);
    }

    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("server: setsockopt");
        exit(1);
    }

    if(listen(sockfd, MAXCLIENTS) < 0){
        perror("server: listen");
        exit(1);
    }

    while(1){
        addr_len = sizeof(cliaddr);
        int newsockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &addr_len);
        if(newsockfd < 0){
            perror("server: accept");
            exit(1);
        }
        printf("server: connection extablished with client at port %d\n", ntohs(cliaddr.sin_port));

        pid_t pid = fork();
        if(pid < 0){
            perror("server: fork");
            exit(1);
        }

        if(pid == 0){
            close(sockfd);
            char client_ip4[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip4, INET_ADDRSTRLEN);

            char filename[INET_ADDRSTRLEN+15], encfile[INET_ADDRSTRLEN+20];
            sprintf(filename,"%s.%d.txt",client_ip4,ntohs(cliaddr.sin_port));
            sprintf(encfile,"%s.enc",filename);
            FILE* fp = NULL;
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
                    int numbytes = recv(newsockfd, buf, MAXBUFLEN, 0);
                    if(numbytes < 0){
                        close(newsockfd);
                        perror("server: recv");
                        exit(1);
                    }
                    else if(numbytes == 0){
                        goto close_conn;
                    }
                    char* eof = memchr(buf, eof_marker, numbytes);
                    numbytes = eof == NULL ? numbytes : eof - buf;

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
                    fflush(fp);

                    if(eof != NULL) break;
                }

                fclose(fp);

                FILE* plain = fopen(filename,"r");
                FILE* cipher = fopen(encfile,"w");
                char ch;
                while((ch = fgetc(plain)) != EOF) fputc(substitute_char(ch,key),cipher);
                fclose(plain);
                fclose(cipher);

                fp = fopen(encfile,"r");
                size_t bytes_read;
                while((bytes_read = fread(chunk, 1, CHUNKSIZE, fp)) > 0){
                    if(send(newsockfd, chunk, bytes_read, 0) < 0){
                        perror("server: send");
                        fclose(fp);
                        close(sockfd);
                        exit(1);
                    }
                }
                send(newsockfd, &eof_marker, 1, 0);
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
            close_conn:
            printf("server: connection closed by client at port %d\n", ntohs(cliaddr.sin_port));
            exit(0);
        }
        else{
            close(newsockfd);
        }
    }
}