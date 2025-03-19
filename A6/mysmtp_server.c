#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>

#define MAXCLIENTS 5
#define MAXBUFLEN 300
#define MAXURLLEN 100
#define MAXDATALEN 200

const char* MAILBOX = "mailbox/";
const char SEPERATOR = '.';

// Commands

#define HELO "HELO "
#define MAILFROM "MAIL FROM: "
#define RCPTTO "RCPT TO: "
#define DATA "DATA\n"
#define LIST "LIST "
#define GETMAIL "GET_MAIL "
#define QUIT "QUIT"
// Response codes

typedef struct {
    int code;
    const char *name;
    const char *description;
} ResponseCode;

const ResponseCode SMPT_CODES[] = {
    {200, "OK", "Command executed successfully."},
    {400, "ERR", "Invalid command syntax."},
    {401, "NOT FOUND", "Requested email does not exist."},
    {403, "FORBIDDEN", "Action not permitted."},
    {404, "NOT FOUND", "Requested mailbox is empty or doesn't exist."},
    {500, "SERVER ERROR", "Internal server error."}
};

const int SMPT_CODES_COUNT = sizeof(SMPT_CODES) / sizeof(SMPT_CODES[0]);

const char* responseName(int code){
    for (int i = 0; i < SMPT_CODES_COUNT; i++){
        if (SMPT_CODES[i].code == code) return SMPT_CODES[i].name;  
    }
    return "Unknown status code";    
}

// const char* responseDesc(int code) {
//     for (int i = 0; i < SMPT_CODES_COUNT; i++){
//         if (SMPT_CODES[i].code == code) return SMPT_CODES[i].description;
//     }
//     return "Unknown status code";
// }

int recv_until_null(int sockfd, char* buf){
    int sz = 0;
    while(1){
        int numbytes = recv(sockfd, buf + sz, 1, 0);
        if(numbytes < 0){
            perror("server: recv");
            return -1;
        }
        else if(numbytes == 0){
            printf("server: connection closed by client\n");
            return 0;
        }
        if(buf[sz] == '\0') break;
        sz++;
    }
    // printf("server: Received message: %s\n", buf);
    return sz;
}

int send_response(int sockfd, int code, const char* msg, const char* data){
    char buf[MAXBUFLEN];
    if(msg != NULL) sprintf(buf, "%d %s\n", code, msg);
    else sprintf(buf, "%d %s\n", code, responseName(code));
    if(data != NULL) strcat(buf, data);
    int numbytes = send(sockfd, buf, strlen(buf), 0);
    if(numbytes < 0){
        perror("server: send");
    }
    // printf("server: Sent response: %s", buf);
    return numbytes;
}

void extract_domain(const char *email, char *domain){
    const char* at_ptr = strchr(email,'@');
    if(at_ptr) strcpy(domain, at_ptr+1);
    else domain[0]='\0';
}

bool check_domain(const char* email, const char* domain){
    char edom[MAXURLLEN];
    extract_domain(email, edom);
    return (strcmp(edom, domain)==0);
}

int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    int port = atoi(argv[1]);
    socklen_t addr_len;
    struct sockaddr_in servaddr, cliaddr;
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = INADDR_ANY;
    memset(servaddr.sin_zero, '\0', sizeof(servaddr.sin_zero));

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("server: socket");
        exit(1);
    }

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

    printf("Listening on port %d...\n", port);

    while(1){
        addr_len = sizeof(cliaddr);
        int newsockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &addr_len);
        if(newsockfd < 0){
            perror("server: accept");
            exit(1);
        }
        printf("Client connected: %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

        pid_t pid = fork();
        if(pid < 0){
            perror("server: fork");
            exit(1);
        }

        if(pid == 0){
            close(sockfd);

            char buf[MAXBUFLEN];
            char domain[MAXURLLEN], from[MAXURLLEN], to[MAXURLLEN], listmail[MAXURLLEN];
            char filename[strlen(MAILBOX)+MAXURLLEN+4];
            char message[MAXDATALEN];

            memset(domain, 0, sizeof(domain));
            memset(from, 0, sizeof(from));
            memset(to, 0, sizeof(to));
            memset(listmail, 0, sizeof(listmail));

            while(1){
                int numbytes = recv_until_null(newsockfd, buf);
                if(numbytes <= 0) break;

                if(!memcmp(buf, HELO, strlen(HELO))){
                    if(!strlen(domain)){
                        if(sscanf(buf+strlen(HELO), "%s", domain) != 1){
                            send_response(newsockfd, 400, NULL, NULL);
                        }
                        else{
                            send_response(newsockfd, 200, NULL, NULL);
                            printf("HELO received from %s\n", domain);
                        }
                    }
                    else send_response(newsockfd, 403, "Domain already set", NULL);
                }
                else if(!memcmp(buf, MAILFROM, strlen(MAILFROM))){
                    if(!strlen(domain)){
                        send_response(newsockfd, 403, "Domain not set", NULL);
                    }
                    else if(!strlen(from)){
                        if(sscanf(buf+strlen(MAILFROM), "%s", from) != 1){
                            send_response(newsockfd, 400, NULL, NULL);
                        }
                        else{
                            if(check_domain(from, domain)){
                                send_response(newsockfd, 200, NULL, NULL);
                                printf("%s%s\n", MAILFROM, from);
                            }
                            else{
                                memset(from, 0, sizeof(from));
                                send_response(newsockfd, 403, NULL, NULL);
                            }
                        }
                    }
                    else send_response(newsockfd, 403, "Sender already set", NULL);
                }
                else if(!memcmp(buf, RCPTTO, strlen(RCPTTO))){
                    if(!strlen(domain)){
                        send_response(newsockfd, 403, "Domain not set", NULL);
                    }
                    else if(!strlen(to)){
                        if(sscanf(buf+strlen(RCPTTO), "%s", to) != 1){
                            send_response(newsockfd, 400, NULL, NULL);
                        }
                        else{
                            send_response(newsockfd, 200, NULL, NULL);
                            printf("%s%s\n", RCPTTO, to);
                        }
                    }
                    else send_response(newsockfd, 403, "Recipient already set", NULL);
                }
                else if(!memcmp(buf, DATA, strlen(DATA))){
                    if(!strlen(domain)){
                        send_response(newsockfd, 403, "Domain not set", NULL);
                    }
                    else if(!strlen(from)){
                        send_response(newsockfd, 403, "Sender not set", NULL);
                    }
                    else if(!strlen(to)){
                        send_response(newsockfd, 403, "Recipient not set", NULL);
                    }
                    else{
                        strcpy(message, buf + strlen(DATA));
                        sprintf(filename, "%s%s.txt", MAILBOX, to);

                        FILE* fp = fopen(filename, "a+");
                        if(fp == NULL){
                            perror("server: fopen");
                            send_response(newsockfd, 500, NULL, NULL);
                        }
                        else{
                            time_t t = time(NULL);
                            struct tm tm = *localtime(&t);
                            fprintf(fp, "From: %s\nDate: %02d-%02d-%d\n%s\n%c\n", from, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, message, SEPERATOR);
                            fclose(fp);
                            send_response(newsockfd, 200, "Message stored successfully", NULL);
                            memset(from, 0, sizeof(from));
                            memset(to, 0, sizeof(to));
                            printf("DATA received, message stored\n");
                        }
                    }
                }
                else if(!memcmp(buf, LIST, strlen(LIST))){
                    if(!strlen(domain)){
                        send_response(newsockfd, 403, "Domain not set", NULL);
                    }
                    else{
                        if(sscanf(buf+strlen(LIST), "%s", listmail) != 1){
                            send_response(newsockfd, 400, NULL, NULL);
                        }
                        else if(check_domain(listmail, domain)){
                            printf("%s%s\n", LIST, listmail);
                            sprintf(filename, "%s%s.txt", MAILBOX, listmail);
                            
                            char line[MAXDATALEN], email[MAXURLLEN], date[20];
                            memset(line, 0, sizeof(line));
                            memset(email, 0, sizeof(email));
                            memset(date, 0, sizeof(date));
                            FILE* fp = fopen(filename, "r");
                            if(fp == NULL){                                
                                perror("server: fopen");
                                if(errno == ENOENT) send_response(newsockfd, 404, NULL, NULL);
                                else send_response(newsockfd, 500, NULL, NULL);
                            }
                            else{
                                int count = 0;
                                int sz = 0;
                                while(fgets(line, sizeof(line), fp)){
                                    if(!strlen(email)){
                                        sscanf(line, "From: %s", email);
                                    }
                                    else if(!strlen(date)){
                                        sscanf(line, "Date: %s", date);
                                    }
                                    else if(!strcmp(line, ".\n")){
                                        count++;
                                        sprintf(line, "%d: Email from %s (%s)\n", count, email, date);
                                        memcpy(buf+sz, line, strlen(line));
                                        sz += strlen(line);
                                        memset(email, 0, sizeof(email));
                                        memset(date, 0, sizeof(date));
                                    }
                                }
                                fclose(fp);  
                                buf[sz] = '\0';
                                send_response(newsockfd, 200, NULL, buf);
                                printf("Emails retrieved; list sent\n");
                            }
                        }
                        else{
                            memset(listmail, 0, sizeof(listmail));
                            send_response(newsockfd, 403, NULL, NULL);                            
                        }
                    }
                }
                else if(!memcmp(buf, GETMAIL, strlen(GETMAIL))){
                    if(!strlen(domain)){
                        send_response(newsockfd, 403, "Domain not set", NULL);
                    }
                    else{
                        int id;
                        if(sscanf(buf+strlen(GETMAIL), "%s %d", listmail, &id) != 2){
                            send_response(newsockfd, 400, NULL, NULL);
                        }
                        else if(check_domain(listmail, domain)){
                            printf("%s%s %d\n", GETMAIL, listmail, id);
                            sprintf(filename, "%s%s.txt", MAILBOX, listmail);
                            FILE* fp = fopen(filename, "r");
                            if(fp == NULL){
                                perror("server: fopen");
                                if(errno == ENOENT) send_response(newsockfd, 404, NULL, NULL);
                                else send_response(newsockfd, 500, NULL, NULL);
                            }
                            else{
                                char line[MAXDATALEN];
                                memset(line, 0, sizeof(line));
                                int count = 0;
                                int sz = 0;
                                while(fgets(line, sizeof(line), fp)){
                                    if(!strcmp(line, ".\n")){
                                        count++;
                                        if(count == id){
                                            buf[sz] = '\0';
                                            send_response(newsockfd, 200, NULL, buf);
                                            printf("Email with ID %d sent\n", id);
                                            break;
                                        }
                                        else{
                                            memset(buf, 0, sizeof(buf));
                                            sz = 0;
                                        }
                                    }
                                    else{
                                        memcpy(buf+sz, line, strlen(line));
                                        sz += strlen(line);
                                    }
                                }
                                fclose(fp);
                                if(count < id){
                                    send_response(newsockfd, 401, NULL, NULL);
                                }
                            }
                        }
                        else{
                            memset(listmail, 0, sizeof(listmail));
                            send_response(newsockfd, 403, NULL, NULL);
                        }
                    }
                }
                else if(!memcmp(buf, QUIT, strlen(QUIT))){
                    send_response(newsockfd, 200, "Goodbye", NULL);
                    printf("Client disconnected: %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
                    break;
                }
                else{
                    send_response(newsockfd, 400, "Syntax Error", NULL);
                }
            }
            close(newsockfd);
            exit(0);
        }
        else{
            close(newsockfd);
        }
    }
}