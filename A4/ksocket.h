#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/syscall.h>
#include <signal.h>

typedef int ksockfd_t;
typedef int usockfd_t;

#define MSGSIZE 512                                  // Bytes
#define MSGTYPE 4                                    // BytesInHeader
#define HEADERSIZE (MSGTYPE + 2 * sizeof(u_int16_t)) // Bytes
#define PACKETSIZE (HEADERSIZE + MSGSIZE)            // Bytes
#define SEQSIZE 8                                    // Bits
#define MAXSEQ (1 << SEQSIZE)                        // Sequence Space
#define BUFFSIZE 10                                  // Messages
#define QUEUEMAXLEN BUFFSIZE
#define WINDOWSIZE BUFFSIZE
#define T 5  // Timeout Seconds
#define N 10 // Number of active sockets
#define P 0.3

#define SOCK_KTP SOCK_DGRAM // Socket type

// Error codes
#define ENOSPACE ENOSPC
#define ENOTBOUND ENOTCONN
#define ENOMESSAGE ENOMSG

#define SHM_PATH "/ktp_shm"
#define SHM_ID 'K'
#define SEM_PATH "/ktp_sem"
#define SEM_ID 'K'

typedef struct window
{
    int base;
    u_int16_t size;
    u_int16_t msg_seq[WINDOWSIZE];
    u_int16_t last_ack;         // For rwnd
    u_int16_t last_seq;         // For swnd
    bool received[WINDOWSIZE];  // For rwnd
    time_t timeout[WINDOWSIZE]; // For swnd
} window;

typedef struct queue
{
    char *buff[QUEUEMAXLEN];
    int len[QUEUEMAXLEN];
    int front;
    int rear;
} queue;

typedef struct k_sockinfo
{
    bool is_free;                      // whether the KTP socket is free or allotted
    pid_t pid;                         // Process ID for the process that created the KTP socket
    usockfd_t sockfd;                  // mapping from the KTP socket to the corresponding UDP socket
    struct sockaddr_in dest_addr;      // IP & Port address of the other end of the KTP socket
    struct sockaddr_in src_addr;       // IP & Port address of the KTP socket
    bool is_bound;                     // whether the KTP socket is bound to an IP & Port
    char send_buff[BUFFSIZE][MSGSIZE]; // Send buffer for the KTP socket
    char recv_buff[BUFFSIZE][MSGSIZE]; // Receive buffer for the KTP socket
    bool send_buff_empty[BUFFSIZE];    // Whether the corresponding message in the send buffer is empty
    window swnd;                       // Send window for the KTP socket, that contains the seq no's of the messages sent but not yet acknowledged
    window rwnd;                       // Receive window for the KTP socket, indicating the seq no's expected by the receiver
    bool nospace;                      // whether the KTP socket has no space in the recv buffer
} k_sockinfo;

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// User Functions

/*
Opens an UDP socket with the socket() call. The parameters to these are the same as the normal socket() call, except that it will take only
SOCK_KTP as the socket type. Checks whether a free space is available in the SM, creates the corresponding UDP socket if a free space is
available, and initializes SM with corresponding entries. If no free space is available, it returns -1 with the global error variable set to ENOSPACE.
*/
ksockfd_t k_socket(int domain, int type, int protocol);

/*
Binds the socket with some address-port using the bind() call. Bind is necessary for each KTP socket irrespective of whether it is used as a server or a
client. This function takes the source IP, the source port, the destination IP and the destination port.
It binds the UDP socket with the source IP and source port, and updates the corresponding SM with the destination IP and destination port.
*/
int k_bind(ksockfd_t sockfd, const char *src_ip, int src_port, const char *dest_ip, int dest_port);

/*
Writes the message to the sender side message buffer if the destination IP/Port matches with the bounded IP/Port as set through k_bind().
If not, it drops the message, returns -1 and sets the global error variable to ENOTBOUND.
If there is no space in the send buffer, return -1 and set the global error variable to ENOSPACE.
*/
ssize_t k_sendto(ksockfd_t sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);

/*
Looks up the receiver-side message buffer to see if any message is already received.
If yes, it returns the first message and deletes that message from the table.
If not, it returns -1 and sets a global error variable to ENOMESSAGE, indicating no message has been available in the message buffer.
*/
ssize_t k_recvfrom(ksockfd_t sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

/*
Closes the socket and cleans up the corresponding socket entry in the SM and marks the entry as free.
*/
int k_close(ksockfd_t fd);

// Internal Functions

// Shared Memory
int k_shmget();
k_sockinfo *k_shmat();
int k_shmdt(k_sockinfo *);

// Semaphore
int k_semget();
void wait_sem(int semid, ksockfd_t i);
void signal_sem(int semid, ksockfd_t i);

// Window
window init_window();

/*
* Simulate unreliable links
generates a random number between 0 and 1. If the generated number is < p, then the function returns 1, else it returns 0
*/
bool dropMessage(float p);