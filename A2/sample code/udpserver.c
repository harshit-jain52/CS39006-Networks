/* CSE39006: Sample Program
 * A simple Datagram socket server
 */

#include <stdio.h> 
#include <strings.h> 
#include <sys/types.h> 
#include <arpa/inet.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h>
#include <stdlib.h>

#define PORT 5000 
#define MAXLINE 1000 
  
int main() 
{    
    char buffer[100]; 
    char *message = "Hello Client"; 
    int serverfd; 
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr; 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // Create a UDP Socket 
    serverfd = socket(AF_INET, SOCK_DGRAM, 0);         
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_family = AF_INET;  
   
    // bind server address to socket descriptor 
    bind(serverfd, (struct sockaddr*)&servaddr, sizeof(servaddr)); 
    
    printf("\nServer Running .........\n");
   
    //receive the datagram 
    len = sizeof(cliaddr);
    int n = recvfrom(serverfd, buffer, sizeof(buffer), 
            0, (struct sockaddr*)&cliaddr, &len); //receive message from server 
    buffer[n] = '\0'; 
    printf("\nReceived from Client: %s\n",buffer); 
           
    // send the response 
    sendto(serverfd, message, MAXLINE, 0, 
          (struct sockaddr*)&cliaddr, sizeof(cliaddr));
    printf("\nMessage sent to client"); 

    close(serverfd);

    return 0;
} 
