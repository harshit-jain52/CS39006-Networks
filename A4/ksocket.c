#include "ksocket.h"

ksockfd_t k_socket(int domain, int type, int protocol)
{
    if (type != SOCK_KTP)
    {
        return -1;
    }

    k_sockinfo *SM = k_shmat();
    if (SM == NULL)
    {
        return -1;
    }

    int semid = k_semget();
    if (semid == -1)
    {
        return -1;
    }

    for (int i = 0; i < N; i++)
    {
        wait_sem(semid, i);
        if (SM[i].is_free)
        {
            usockfd_t sockfd = socket(domain, SOCK_DGRAM, protocol);
            if (sockfd < 0)
            {
                signal_sem(semid, i);
                return -1;
            }
            SM[i].sockfd = sockfd;
            SM[i].pid = getpid();
            SM[i].is_free = false;
            bzero(&SM[i].dest_addr, sizeof(SM[i].dest_addr));
            for (int j = 0; j < BUFFSIZE; j++)
            {

                SM[i].send_buff[j] = NULL;
                SM[i].recv_buff[j] = NULL;
            }
            SM[i].swnd = init_window();
            SM[i].rwnd = init_window();
            SM[i].nospace = false;
            signal_sem(semid, i);
            return i;
        }
        signal_sem(semid, i);
    }

    errno = ENOSPACE;
    return -1;
}

int k_bind(ksockfd_t sockfd, const char *src_ip, int src_port, const char *dest_ip, int dest_port)
{
    k_sockinfo *SM = k_shmat();
    if (SM == NULL)
    {
        return -1;
    }

    int semid = k_semget();
    if (semid == -1)
    {
        return -1;
    }
    wait_sem(semid, sockfd);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(src_port);
    addr.sin_addr.s_addr = inet_addr(src_ip);

    if (bind(SM[sockfd].sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        signal_sem(semid, sockfd);
        return -1;
    }

    SM[sockfd].dest_addr.sin_family = AF_INET;
    SM[sockfd].dest_addr.sin_port = htons(dest_port);
    SM[sockfd].dest_addr.sin_addr.s_addr = inet_addr(dest_ip);

    signal_sem(semid, sockfd);
    return 0;
}

ssize_t k_sendto(ksockfd_t sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    k_sockinfo *SM = k_shmat();
    if (SM == NULL)
    {
        return -1;
    }

    int semid = k_semget();
    if (semid == -1)
    {
        return -1;
    }
    wait_sem(semid, sockfd);

    struct sockaddr_in *dest_addrin = (struct sockaddr_in *)dest_addr;
    if (SM[sockfd].is_free || SM[sockfd].dest_addr.sin_addr.s_addr != dest_addrin->sin_addr.s_addr || SM[sockfd].dest_addr.sin_port != dest_addrin->sin_port)
    {
        errno = ENOTBOUND;
        signal_sem(semid, sockfd);
        return -1;
    }

    for (int j = 0; j < BUFFSIZE; j++)
    {
        if (SM[sockfd].send_buff[j] == NULL)
        {
            SM[sockfd].send_buff[j] = (char *)malloc(MSGSIZE);
            if (SM[sockfd].send_buff[j] == NULL)
            {
                signal_sem(semid, sockfd);
                return -1;
            }
        }

        size_t copybytes = len < MSGSIZE ? len : MSGSIZE;
        memcpy(SM[sockfd].send_buff[j], buf, copybytes);
        signal_sem(semid, sockfd);
        return copybytes;
    }

    signal_sem(semid, sockfd);
    errno = ENOSPACE;
    return -1;
}

ssize_t k_recvfrom(ksockfd_t sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    k_sockinfo *SM = k_shmat();
    if (SM == NULL)
    {
        return -1;
    }

    int semid = k_semget();
    if (semid == -1)
    {
        return -1;
    }
    wait_sem(semid, sockfd);

    int numbytes;

    // TO DO

    signal_sem(semid, sockfd);
    return numbytes;
}

k_sockinfo *k_shmat()
{
    int shmid = k_shmget();

    if (shmid == -1)
    {
        return NULL;
    }

    k_sockinfo *SM = (k_sockinfo *)shmat(shmid, NULL, 0);
    if (SM == (void *)-1)
    {
        return NULL;
    }

    return SM;
}

int k_shmget()
{
    key_t key = ftok(SHM_PATH, SHM_ID);
    return shmget(key, 0, 0);
}

int k_shmdt(k_sockinfo *SM)
{
    return shmdt(SM);
}

int k_semget()
{
    key_t key = ftok(SEM_PATH, SEM_ID);
    return semget(key, 0, 0);
}

void wait_sem(int semid, ksockfd_t i)
{
    struct sembuf sb;
    sb.sem_num = i;
    sb.sem_op = -1; // Lock
    sb.sem_flg = 0;

    if (semop(semid, &sb, 1) == -1)
    {
        perror("wait_sem: semop");
        exit(1);
    }
}

void signal_sem(int semid, ksockfd_t i)
{
    struct sembuf sb;
    sb.sem_num = i;
    sb.sem_op = 1; // Release
    sb.sem_flg = 0;

    if (semop(semid, &sb, 1) == -1)
    {
        perror("signal_sem: semop");
        exit(1);
    }
}

window init_window()
{
    window W;
    W.base = 0;
    W.size = WINDOWSIZE;
    W.last_ack = 0;
    for (int i = 0; i < WINDOWSIZE; i++)
    {
        W.msg_seq[i] = i + 1;
        W.received[i] = false;
    }
    return W;
}