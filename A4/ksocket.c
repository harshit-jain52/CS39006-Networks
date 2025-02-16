#include "ksocket.h"

ksockfd_t k_socket(int domain, int type, int protocol){
    if (type != SOCK_KTP || domain != AF_INET){
        return -1;
    }

    k_sockinfo *SM = k_shmat();
    if (SM == NULL){
        return -1;
    }

    int semid = k_semget();
    if (semid == -1){
        return -1;
    }

    for (int i = 0; i < N; i++){
        wait_sem(semid, i);
        if (SM[i].is_free){
            SM[i].pid = getpid();
            SM[i].is_free = false;
            SM[i].is_bound = false;
            SM[i].is_closed = false;
            bzero(&SM[i].src_addr, sizeof(SM[i].src_addr));
            bzero(&SM[i].dest_addr, sizeof(SM[i].dest_addr));

            for (int j = 0; j < BUFFSIZE; j++){
                SM[i].send_buff_empty[j] = true;
            }
            SM[i].swnd = init_window();
            SM[i].rwnd = init_window();
            SM[i].nospace = false;

            printf("Socket created with ksockfd: %d\n", i);
            signal_sem(semid, i);
            return i;
        }
        signal_sem(semid, i);
    }

    errno = ENOSPACE;
    return -1;
}

int k_bind(ksockfd_t sockfd, const char *src_ip, int src_port, const char *dest_ip, int dest_port){
    k_sockinfo *SM = k_shmat();
    if (SM == NULL){
        return -1;
    }

    int semid = k_semget();
    if (semid == -1){
        return -1;
    }
    wait_sem(semid, sockfd);

    SM[sockfd].src_addr.sin_family = AF_INET;
    SM[sockfd].src_addr.sin_port = htons(src_port);
    SM[sockfd].src_addr.sin_addr.s_addr = inet_addr(src_ip);

    SM[sockfd].dest_addr.sin_family = AF_INET;
    SM[sockfd].dest_addr.sin_port = htons(dest_port);
    SM[sockfd].dest_addr.sin_addr.s_addr = inet_addr(dest_ip);

    printf("Socket bound with ksockfd: %d src_port: %d dest_port: %d\n", sockfd, src_port, dest_port);
    signal_sem(semid, sockfd);
    return 0;
}

ssize_t k_sendto(ksockfd_t sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen){
    k_sockinfo *SM = k_shmat();
    if (SM == NULL){
        return -1;
    }

    int semid = k_semget();
    if (semid == -1){
        return -1;
    }
    wait_sem(semid, sockfd);
    struct sockaddr_in *dest_addrin = (struct sockaddr_in *)dest_addr;
    if (SM[sockfd].is_free || SM[sockfd].dest_addr.sin_addr.s_addr != dest_addrin->sin_addr.s_addr || SM[sockfd].dest_addr.sin_port != dest_addrin->sin_port){
        errno = ENOTBOUND;
        signal_sem(semid, sockfd);
        return -1;
    }

    for (int j = SM[sockfd].swnd.base, ctr = 0; ctr < BUFFSIZE; j = (j + 1) % BUFFSIZE, ctr++){
        if (SM[sockfd].send_buff_empty[j]){
            ssize_t copybytes = len < MSGSIZE ? len : MSGSIZE;
            memcpy(SM[sockfd].send_buff[j], buf, copybytes);
            for (int i = len; i < MSGSIZE; i++){
                SM[sockfd].send_buff[j][i] = '\0';
            }
            SM[sockfd].send_buff_empty[j] = false;
            SM[sockfd].swnd.timeout[j] = -1;
            printf("k_sendto: Message %s sent with ksockfd: %d seq_no: %d index: %d\n", SM[sockfd].send_buff[j], sockfd, SM[sockfd].swnd.msg_seq[j], j);
            signal_sem(semid, sockfd);
            return copybytes;
        }
    }

    signal_sem(semid, sockfd);
    errno = ENOSPACE;
    return -1;
}

ssize_t k_recvfrom(ksockfd_t sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
    k_sockinfo *SM = k_shmat();
    if (SM == NULL){
        return -1;
    }

    int semid = k_semget();
    if (semid == -1){
        return -1;
    }
    wait_sem(semid, sockfd);

    int numbytes;
    int slot = (SM[sockfd].rwnd.base + SM[sockfd].rwnd.size) % WINDOWSIZE;

    if (SM[sockfd].rwnd.received[slot]){
        memcpy(buf, SM[sockfd].recv_buff[slot], len);
        printf("k_recvfrom: Message %s received with ksockfd: %d seq_no: %d index: %d\n", (char *)buf, sockfd, SM[sockfd].rwnd.msg_seq[slot] - WINDOWSIZE, slot);
        numbytes = strlen((char *)buf);
        SM[sockfd].rwnd.received[slot] = false;
        SM[sockfd].rwnd.size++;
    }
    else{
        numbytes = -1;
        errno = ENOMESSAGE;
    }

    signal_sem(semid, sockfd);
    return numbytes;
}

int k_close(ksockfd_t fd){
    k_sockinfo *SM = k_shmat();
    if (SM == NULL){
        return -1;
    }

    int semid = k_semget();
    if (semid == -1){
        return -1;
    }
    wait_sem(semid, fd);
    SM[fd].is_closed = true;
    printf("k_close: Socket closed with ksockfd: %d\n", fd);
    signal_sem(semid, fd);
    return 0;
}

k_sockinfo *k_shmat(){
    int shmid = k_shmget();

    if (shmid == -1){
        return NULL;
    }

    k_sockinfo *SM = (k_sockinfo *)shmat(shmid, NULL, 0);
    if (SM == (void *)-1){
        return NULL;
    }

    return SM;
}

int k_shmget(){
    key_t key = ftok(SHM_PATH, SHM_ID);
    return shmget(key, 0, 0);
}

int k_shmdt(k_sockinfo *SM){
    return shmdt(SM);
}

int k_semget(){
    key_t key = ftok(SEM_PATH, SEM_ID);
    return semget(key, 0, 0);
}

void wait_sem(int semid, ksockfd_t i){
    struct sembuf sb;
    sb.sem_num = i;
    sb.sem_op = -1; // Lock
    sb.sem_flg = 0;
    // printf("Thread %ld waiting for semaphore %d\n", pthread_self(), i);
    if (semop(semid, &sb, 1) == -1){
        perror("wait_sem: semop");
        exit(1);
    }
}

void signal_sem(int semid, ksockfd_t i){
    struct sembuf sb;
    sb.sem_num = i;
    sb.sem_op = 1; // Release
    sb.sem_flg = 0;
    // printf("Thread %ld signalling semaphore %d\n",  pthread_self(), i);
    if (semop(semid, &sb, 1) == -1){
        perror("signal_sem: semop");
        exit(1);
    }
}

window init_window(){
    window W;
    W.base = 0;
    W.size = WINDOWSIZE;
    W.last_ack = 0;
    W.last_seq = 10;
    for (int i = 0; i < WINDOWSIZE; i++){
        W.msg_seq[i] = i + 1;
        W.received[i] = false;
        W.timeout[i] = -1;
    }
    return W;
}

bool dropMessage(float p){
    float r = (float)rand() / (float)RAND_MAX;
    return r < p;
}