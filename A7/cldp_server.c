/*
===================================== 
Assignment 7 Submission - cldp_server.c
Name: Harshit Jain
Roll number: 22CS10030
===================================== 
*/

#include "cldp.h"

#define WAITTIME 10
#define SLEEPTIME 800000

in_addr_t get_local_ip() {
    struct ifaddrs *ifaddr, *ifa;
    in_addr_t ip = htonl(INADDR_LOOPBACK);
    
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return ip;
    }
    
    // Walk through linked list of interfaces
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
            
        // Check for IPv4 and skip loopback
        if (ifa->ifa_addr->sa_family == AF_INET && 
            strcmp(ifa->ifa_name, "lo") != 0) {
            struct sockaddr_in *addr = (struct sockaddr_in*)ifa->ifa_addr;
            ip = addr->sin_addr.s_addr;
            break;
        }
    }
    
    freeifaddrs(ifaddr);
    return ip;
}

void *Broadcast(void* targ){
    int sock = *(int *)targ;
    struct sockaddr_in broadcast_addr;
    char buff[MAXBUFLEN];
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    in_addr_t local_ip = get_local_ip();

    for(;;){
        memset(buff, 0, sizeof(buff));

        struct cldphdr* clpd = (struct cldphdr*)(buff + sizeof(struct iphdr));
        clpd->type = 0x01; // HELLO
        clpd->payload_len = 0;
        clpd->trans_id = 0;
        memset(clpd->reserved, 0, sizeof(clpd->reserved));

        struct iphdr* ip = (struct iphdr*)(buff);
        fill_defaults(ip);
        ip->tot_len = sizeof(struct cldphdr) + sizeof(struct iphdr);
        ip->saddr = local_ip;
        ip->daddr = htonl(INADDR_BROADCAST);
        ip->check = csum((unsigned short*)buff, ip->tot_len);

        int numbytes = sendto(sock, buff, ip->tot_len, 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
        if(numbytes < 0){
            perror("sendto");
            continue;
        }
        printf("Broadcasting HELLO\n");
        sleep(10);
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

    // Give broadcast permission
    opt = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0){
        perror("setsockopt");
        exit(1);
    }

    // Thread to broadcast HELLO every 10 seconds
    pthread_t bid;
    int* sockfd = (int*)malloc(sizeof(int));
    *sockfd = sock;
    pthread_create(&bid, NULL, Broadcast, sockfd);

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

        struct cldphdr* clpd = (struct cldphdr*)(buf + sizeof(struct iphdr));
        if(clpd->type == HELLO){
            // IGNORE
            continue;
        }
        else if(clpd->type == QUERY){
            int trans_id = clpd->trans_id;
            printf("Received QUERY from %s, transaction id: %d\n", inet_ntoa(*(struct in_addr*)&cli_addr.sin_addr), trans_id);
            memset(buf, 0, sizeof(buf));
            struct iphdr* ip = (struct iphdr*)(buf);
            struct cldphdr* clpd = (struct cldphdr*)(buf + sizeof(struct iphdr));
            char* payload = (char*)(buf + sizeof(struct iphdr) + sizeof(struct cldphdr));
            struct Response* response = (struct Response*)malloc(sizeof(struct Response));
            gethostname(response->hostname, 256);
            struct timeval tv;
            gettimeofday(&tv, NULL);
            struct tm* tm_info = localtime(&tv.tv_sec);
            strftime(response->system_time, sizeof(response->system_time), "%Y-%m-%d %H:%M:%S", tm_info);
            sysinfo(&response->sys_info);
            memcpy(payload, response, sizeof(struct Response));
            free(response);

            // Fill the CLDP header
            clpd->type = 0x03; // RESPONSE
            clpd->payload_len = sizeof(struct Response);
            clpd->trans_id = trans_id;
            memset(clpd->reserved, 0, sizeof(clpd->reserved));

            // Fill the IP header
            fill_defaults(ip);
            ip->tot_len = sizeof(struct cldphdr) + sizeof(struct iphdr) + sizeof(struct Response);
            ip->saddr = htonl(INADDR_ANY);
            ip->daddr = cli_addr.sin_addr.s_addr;
            ip->check = csum((unsigned short*)buf, ip->tot_len);
            int numbytes = sendto(sock, buf, ip->tot_len, 0, (struct sockaddr*)&cli_addr, addr_len);
            if(numbytes < 0){
                perror("sendto");
                continue;
            }
        }
    }
}