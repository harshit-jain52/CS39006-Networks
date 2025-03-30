/*
===================================== 
Assignment 7 Submission - cldp_client.c
Name: Harshit Jain
Roll number: 22CS10030
===================================== 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define IPPROTO_CLDP 253
#define WAITTIME 8
#define SLEEPTIME 800000
#define MAXBUFLEN 2048
#define HELLO 0x01
#define QUERY 0x02
#define RESPONSE 0x03

struct cldphdr{
    unsigned char type;
    unsigned int payload_len;
    unsigned int trans_id;
    char reserved[8];
};

struct Response{
    char hostname[256];
    char system_time[64];
};

// Linked List of servers
typedef struct ServerLL{
    struct in_addr addr;
    struct ServerLL* next;
} ServerNode;

// Linked List of requests
typedef struct IDLL{
    unsigned int trans_id;
    ServerNode* server;
    struct IDLL* next;
} IdNode;

ServerNode* server_head = NULL;
IdNode* idll_head = NULL;
unsigned int trans_id = 0;
pthread_mutex_t lmtx;

ServerNode* makeNode(struct in_addr addr){
    ServerNode* newNode = (ServerNode*)malloc(sizeof(ServerNode));
    newNode->addr = addr;
    newNode->next = NULL;
    return newNode;
}

IdNode* makeIdNode(ServerNode* server){
    IdNode* newIdNode = (IdNode*)malloc(sizeof(IdNode));
    newIdNode->trans_id = ++trans_id;
    newIdNode->server = server;
    newIdNode->next = NULL;
    return newIdNode;
}

void addNode(struct in_addr addr){
    ServerNode* newNode = makeNode(addr);
    if(server_head == NULL){
        server_head = newNode;
    } else{
        ServerNode* temp = server_head;
        while(temp != NULL){
            if(temp->addr.s_addr == addr.s_addr){
                free(newNode);
                return;
            }
            temp = temp->next;
        }
        newNode->next = server_head;
        server_head = newNode;
    }
    printf("Added server %s\n", inet_ntoa(addr));
}

IdNode* addIdNode(ServerNode* server){
    IdNode* newIdNode = makeIdNode(server);
    if(idll_head == NULL){
        idll_head = newIdNode;
    } else {
        IdNode* temp = idll_head;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = newIdNode;
    }
    return newIdNode;
}

IdNode* searchId(int id){
    IdNode* temp = idll_head;
    while(temp != NULL){
        if(temp->trans_id == id){
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

void deleteIdNode(int id){    
    IdNode* temp = idll_head;
    IdNode* prev = NULL;
    while(temp != NULL){
        if(temp->trans_id == id){
            if(prev == NULL){
                idll_head = temp->next;
            } else {
                prev->next = temp->next;
            }
            free(temp);
            return;
        }
        prev = temp;
        temp = temp->next;
    }
}

void *Query(void *targ){
    int sock = *(int *)targ;
    char buff[MAXBUFLEN];

    for(;;){
        sleep(WAITTIME);
        ServerNode* temp = server_head;
        while(temp != NULL){
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr = temp->addr;
            pthread_mutex_lock(&lmtx);
            IdNode* idNode = addIdNode(temp);
            pthread_mutex_unlock(&lmtx);
            // Fill the CLDP header
            struct cldphdr* clpd = (struct cldphdr*)(buff + sizeof(struct iphdr));
            clpd->type = QUERY;
            clpd->payload_len = 0;
            clpd->trans_id = idNode->trans_id;
            memset(clpd->reserved, 0, sizeof(clpd->reserved));
            // Fill the IP header
            struct iphdr* ip = (struct iphdr*)(buff);
            ip->version = 4;
            ip->ihl = 5;
            ip->tot_len = sizeof(struct cldphdr) + sizeof(struct iphdr);
            ip->ttl = 255;
            ip->protocol = IPPROTO_CLDP;
            ip->saddr = htonl(INADDR_ANY);
            ip->daddr = temp->addr.s_addr;

            int numbytes = sendto(sock, buff, ip->tot_len, 0, (struct sockaddr*)&addr, sizeof(addr));
            if(numbytes < 0){
                perror("sendto");
                continue;
            }
            temp = temp->next;
        }
    }
}

int main(){
    struct sockaddr_in cli_addr;
    socklen_t addr_len;
    char buf[MAXBUFLEN];

    // Raw socket
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_CLDP);
    if(sock < 0){
        perror("socket");
        exit(1);
    }

    // Make socket non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    if(flags < 0){
        perror("fcntl F_GETFL");
        exit(1);
    }
    if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0){
        perror("fcntl F_SETFL");
        exit(1);
    }

    // Inform the kernel that the header is included in the data, so don't insert its own header into the packet before the data
    int opt = 1;
    if(setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0){
        perror("setsockopt");
        exit(1);
    }
    
    // Thread to QUERY regularly
    pthread_t qid;
    pthread_mutex_init(&lmtx, NULL);
    int* sockfd = (int*)malloc(sizeof(int));
    *sockfd = sock;
    pthread_create(&qid, NULL, Query, sockfd);

    while(1){
        addr_len = sizeof(cli_addr);
        int numbytes = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&cli_addr, &addr_len);
        if(numbytes < 0){
            if(errno == EWOULDBLOCK || errno == EAGAIN){
                usleep(SLEEPTIME);
                continue;
            }
            perror("recvfrom");
            exit(1);
        }
        struct iphdr* ip = (struct iphdr*)buf;
        struct cldphdr* clpd = (struct cldphdr*)(buf + ip->ihl*4);
        if(clpd->type==HELLO){
            printf("Received HELLO from %s\n", inet_ntoa(*(struct in_addr*)&ip->saddr));
            pthread_mutex_lock(&lmtx);
            addNode(*(struct in_addr*)&ip->saddr);
            pthread_mutex_unlock(&lmtx);
        }
        else if(clpd->type==RESPONSE){
            pthread_mutex_lock(&lmtx);
            IdNode* idNode = searchId(clpd->trans_id);
            pthread_mutex_unlock(&lmtx);
            if(idNode == NULL || (idNode->server->addr.s_addr != cli_addr.sin_addr.s_addr)){
                printf("Received malformed RESPONSE from %s\n", inet_ntoa(*(struct in_addr*)&ip->saddr));
            }
            else{
                printf("Received RESPONSE from %s, transaction id: %d\n", inet_ntoa(*(struct in_addr*)&ip->saddr), clpd->trans_id);
                struct Response* resp = (struct Response*)(buf + ip->ihl*4 + sizeof(struct cldphdr));
                printf("Hostname: %s\t", resp->hostname);
                printf("System Time: %s\n", resp->system_time);
                pthread_mutex_lock(&lmtx);
                deleteIdNode(idNode->trans_id);
                pthread_mutex_unlock(&lmtx);
            }
        }
    }
}