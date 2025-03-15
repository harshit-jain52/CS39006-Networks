/*
===================================== 
Assignment 5 Submission - taskqueueserver.c
Name: Harshit Jain
Roll number: 22CS10030
===================================== 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 8008
#define MAXTASKS 100
#define MAXCLIENTS 5
#define SEMNAME "/qsem"
#define SHMPATH "qshm"
#define SLEEPTIME 800000
#define NO_DATA_LIMIT 15
#define GET_TASK "GET_TASK"
#define RESULT "RESULT\0\0"
#define EXIT "exit\0\0\0\0"
#define HEADER_SIZE 8
#define MSG_BYTES (HEADER_SIZE + sizeof(int))
#define MAXBUFLEN 100
#define PENDING_TASK "TASK PENDING"
#define NOT_AVAILABLE "No tasks available"

void sig_handler(int signo){
    int shmid = shmget(ftok(SHMPATH, 'Q'), 0, 0);
    shmctl(shmid, IPC_RMID, NULL);
    sem_unlink(SEMNAME);
    exit(0);
}

void cleanup(int sockfd, sem_t* sem, void* SM){
    close(sockfd);
    sem_close(sem);
    sem_unlink(SEMNAME);
    int shmid = shmget(ftok(SHMPATH, 'Q'), 0, 0);
    shmdt(SM);
    shmctl(shmid, IPC_RMID, NULL);
}

int make_socket_nonblocking(int sockfd){
    int flags = fcntl(sockfd, F_GETFL, 0);
    if(flags < 0){
        perror("server: fcntl F_GETFL");
        return -1;
    }
    if(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0){
        perror("server: fcntl F_SETFL");
        return -1;
    }
    return 0;
}

int recv_nonblock(int sockfd, char* buf, int sz, struct sockaddr_in* cliaddr){
    int nodata = 0;
    int numbytes = 0;
    while(1){
        numbytes = recv(sockfd, buf, sz, 0);
        if(numbytes < 0){
            if(errno == EWOULDBLOCK || errno == EAGAIN){
                // printf("server: no incoming data from client at port %d\n", ntohs(cliaddr->sin_port));
                nodata++;
                if(nodata == NO_DATA_LIMIT){
                    printf("server: no-data limit reached for client at port %d. Closing connection.\n", ntohs(cliaddr->sin_port));
                    break;
                }
                usleep(SLEEPTIME);
                continue;
            } else {
                perror("server: recv");
                break;
            }
        }
        else if(numbytes == 0){
            printf("server: connection closed by client at port %d\n", ntohs(cliaddr->sin_port));
            break;
        }
        else break;
    }
    return numbytes;
}

typedef struct Task{
    int num1;
    int num2;
    char op;
} Task;

typedef struct TaskQueue{
    Task tasks[MAXTASKS];
    int front;
    int rear;
} TaskQueue;

TaskQueue* init_shm(){
    key_t key = ftok(SHMPATH, 'Q');
    if(key < 0){
        perror("server: ftok");
        exit(1);
    }
    
    int shmid = shmget(key, sizeof(TaskQueue), IPC_CREAT | IPC_EXCL | 0777);
    if(shmid < 0){
        perror("server: shmget");
        exit(1);
    }

    TaskQueue* taskQueue = (TaskQueue*)shmat(shmid, NULL, 0);
    if(taskQueue == (TaskQueue*)-1){
        perror("server: shmat");
        exit(1);
    }

    taskQueue->front = 0;
    taskQueue->rear = 0;
    return taskQueue;
}

int main(){
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    struct sockaddr_in servaddr, cliaddr;
    sem_t* sem;
    TaskQueue* TQ;
    FILE* fp;
    
    // Signal Handling
    signal(SIGINT, sig_handler);
    signal(SIGSEGV, sig_handler);
    
    // Semaphore Initialization
    sem = sem_open(SEMNAME, O_CREAT | O_EXCL, 0666, 1);
    if(sem == SEM_FAILED){
        perror("server: sem_open");
        exit(1);
    }

    // Shared Memory Initialization
    TQ = init_shm();
    fp = fopen("tasks.config", "r");
    if(fp == NULL){
        perror("server: fopen");
        exit(1);
    }

    while(fgets(buf, MSG_BYTES, fp) != NULL){
        Task task;
        sscanf(buf, "%d %c %d", &task.num1, &task.op, &task.num2);
        sem_wait(sem);
        TQ->tasks[TQ->rear] = task;
        TQ->rear = (TQ->rear + 1) % MAXTASKS;
        sem_post(sem);
    }
    fclose(fp);

    // TCP Socket
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

    if(make_socket_nonblocking(sockfd) < 0){
        exit(1);
    }

    // Allow to reuse the port when rerun
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("server: setsockopt");
        exit(1);
    }

    if(listen(sockfd, MAXCLIENTS) < 0){
        perror("server: listen");
        exit(1);
    }

    printf("server: listening on port %d\n", PORT);

    while(1){
        pid_t wait_pid;
        while((wait_pid = waitpid(-1, NULL, WNOHANG)) > 0){
            printf("server: child %d killed\n", wait_pid);
        }

        int newsockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &addr_len);
        if(newsockfd < 0){
            if(errno == EWOULDBLOCK || errno == EAGAIN){
                // printf("server: no incoming connections\n");
                usleep(SLEEPTIME*2);
                continue;
            } else {
                perror("server: accept");
                break;
            }
        }

        printf("server: connection established with client at port %d\n", ntohs(cliaddr.sin_port));

        pid_t pid = fork();
        if(pid < 0){
            perror("server: fork");
            break;
        }
        if(pid==0){
            close(sockfd);
            if(make_socket_nonblocking(newsockfd) < 0){
                exit(1);
            }
            bool task_pending = false;
            while(1){
                int sz = 0;
                while(sz<HEADER_SIZE){
                    int numbytes = recv_nonblock(newsockfd, buf + sz, HEADER_SIZE - sz, &cliaddr);
                    if(numbytes <= 0){
                        break;
                    }
                    sz += numbytes;
                }
                if(sz < HEADER_SIZE) break; 
                char type[HEADER_SIZE];
                memcpy(type, buf, HEADER_SIZE);
                int sendbytes = 0;
                if(!memcmp(type, GET_TASK, HEADER_SIZE)){
                    printf("server: GET_TASK request from client at port %d\n", ntohs(cliaddr.sin_port));
                    if(!task_pending){
                        sem_wait(sem);
                        if(TQ->front == TQ->rear){
                            sem_post(sem);
                            sprintf(buf, NOT_AVAILABLE);
                            buf[strlen(buf)] = '\0';
                            sendbytes = send(newsockfd, buf, strlen(buf)+1, 0);
                        }
                        else{
                            Task task = TQ->tasks[TQ->front];
                            TQ->front = (TQ->front + 1) % MAXTASKS;
                            sem_post(sem);
                            sprintf(buf, "Task: %d %c %d", task.num1, task.op, task.num2);
                            buf[strlen(buf)] = '\0';
                            sendbytes = send(newsockfd, buf, strlen(buf)+1, 0);
                            task_pending = true;
                        }
                    }
                    else{
                        sprintf(buf, PENDING_TASK);
                        buf[strlen(buf)] = '\0';
                        sendbytes = send(newsockfd, buf, strlen(buf)+1, 0);
                    }
                }
                else if(!memcmp(type, RESULT, HEADER_SIZE)){
                    while(sz<MSG_BYTES){
                        int numbytes = recv_nonblock(newsockfd, buf + sz, MSG_BYTES - sz, &cliaddr);
                        if(numbytes <= 0){
                            break;
                        }
                        sz += numbytes;
                    }
                    if(sz < MSG_BYTES) break;
                    if(task_pending){
                        task_pending = false;
                        int res;
                        memcpy(&res, buf + HEADER_SIZE, sizeof(int));
                        printf("server: RESULT %d received from client at port %d\n", res, ntohs(cliaddr.sin_port));
                        sprintf(buf, "Result received");
                        buf[strlen(buf)] = '\0';
                        sendbytes = send(newsockfd, buf, strlen(buf)+1, 0);
                    }
                    else{
                        sprintf(buf, "Request a new task first");
                        buf[strlen(buf)] = '\0';
                        sendbytes = send(newsockfd, buf, strlen(buf)+1, 0);
                    }
                }
                else if(!memcmp(type, EXIT, HEADER_SIZE)){
                    printf("server: EXIT signal from client at port %d\n", ntohs(cliaddr.sin_port));
                    break;
                }
                else{
                    sprintf(buf, "Invalid request");
                    buf[strlen(buf)] = '\0';
                    sendbytes = send(newsockfd, buf, strlen(buf)+1, 0);
                }
                if(sendbytes < 0){
                    perror("server: send");
                    break;
                }
            }
            close(newsockfd);
            exit(0);
        }
        else{
            close(newsockfd);
        }
    }

    cleanup(sockfd, sem, TQ);
    return 0;
}