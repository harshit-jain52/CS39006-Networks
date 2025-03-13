#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define SERVERPORT 8008
#define MAXBUFLEN 100
#define MAXTASKS 100
#define MAXCLIENTS 5
#define SEMNAME "/qsem"
#define SHMPATH "qshm"
#define SLEEPTIME 800000
#define NO_DATA_LIMIT 10
#define GET_TASK "GET_TASK"
#define RESULT "RESULT\0\0"
#define EXIT "exit\0\0\0\0"
#define HEADER_SIZE 8
#define MSG_BYTES (HEADER_SIZE + sizeof(int))
#define PENDING_TASK "TASK PENDING"
#define NOT_AVAILABLE "No tasks available"


void print_menu(){
    printf("--------------------\n");
    printf("1. Get Task\n");
    printf("2. Send Result\n");
    printf("3. Exit\n");
    printf("Enter choice: ");
}

int recv_until_null(int sockfd, char* buf){
    int sz = 0;
    while(1){
        int numbytes = recv(sockfd, buf + sz, 1, 0);
        if(numbytes < 0){
            perror("client: recv");
            return -1;
        }
        else if(numbytes == 0){
            printf("client: connection closed by server\n");
            return 0;
        }
        if(buf[sz] == '\0') break;
        sz++;
    }
    printf("client: Received message: %s\n", buf);
    return sz;
}

int execute_task(int num1, int num2, char op){
    if(op == '/' && num2 == 0){
        printf("client: Division by zero\n");
        return -1;
    }
    switch(op){
        case '+': return num1 + num2;
        case '-': return num1 - num2;
        case '*': return num1 * num2;
        case '/': return num1 / num2;
        default: return -1;
    }
}

int main(){
    char buf[MAXBUFLEN];
    struct sockaddr_in servaddr;
    
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("client: socket");
        exit(1);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVERPORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        perror("client: connect");
        exit(1);
    }
    printf("client: connection established with server\n");

    int res = 0;
    while(1){
        int choice;
        print_menu();
        scanf("%d", &choice);
        char type[HEADER_SIZE];
        if(choice == 1){
            memcpy(type, GET_TASK, HEADER_SIZE);
            int n = send(sockfd, type, HEADER_SIZE, 0);
            if(n < 0){
                perror("client: send");
                break;
            }
            int numbytes = recv_until_null(sockfd, buf);
            if(numbytes <= 0) break;
            if(!memcmp(buf, PENDING_TASK, numbytes)) continue;
            if(!memcmp(buf, NOT_AVAILABLE, numbytes)){
                printf("client: Exiting\n");
                memcpy(type, EXIT, HEADER_SIZE);
                send(sockfd, type, HEADER_SIZE, 0);
                break; 
            }
            int num1, num2;
            char op;
            sscanf(buf, "Task: %d %c %d", &num1, &op, &num2);
            res = execute_task(num1, num2, op);
            printf("client: Task executed: %d %c %d = %d\n", num1, op, num2, res);
        }
        else if(choice == 2){
            memcpy(buf, RESULT, HEADER_SIZE);
            memcpy(buf + HEADER_SIZE, &res, sizeof(int));
            int n = send(sockfd, buf, MSG_BYTES, 0);
            if(n < 0){
                perror("client: send");
                break;
            }
            int numbytes = recv_until_null(sockfd, buf);
            if(numbytes <= 0) break;
        }
        else if(choice == 3){
            memcpy(type, EXIT, HEADER_SIZE);
            send(sockfd, type, HEADER_SIZE, 0);
            break;
        }
        else{
            printf("Invalid choice\n");
        }
    }

    close(sockfd);
    return 0;
}