/*
===================================== 
Assignment 7 Submission - cldp.h
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
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/time.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/types.h>

#define IPPROTO_CLDP 169
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
    struct sysinfo sys_info;
};

void fill_defaults(struct iphdr* ip){
    ip->version = 4;
    ip->ihl = 5;
    ip->ttl = 255;
    ip->protocol = IPPROTO_CLDP;
}

unsigned short csum(unsigned short *ptr,int nbytes) 
{
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum=0;
	while(nbytes>1) {
		sum+=*ptr++;
		nbytes-=2;
	}
	if(nbytes==1) {
		oddbyte=0;
		*((__u_char*)&oddbyte)=*(__u_char*)ptr;
		sum+=oddbyte;
	}

	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);
	answer=(short)~sum;
	
	return(answer);
}
