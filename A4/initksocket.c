#include "ksocket.h"

/*
Initializes two threads R and S, and a shared memory SM.
Thread R handles all messages received from the UDP socket, and thread S handles the timeouts and retransmissions.
Also starts a garbage collector process for cleanup.
*/

void initk_shm()
{
    key_t key = ftok(SHM_PATH, SHM_ID);
    int shmid = shmget(key, N * sizeof(k_sockinfo), IPC_CREAT | 0777 | IPC_EXCL);
    if (shmid == -1)
    {
        perror("initsocket: shmget");
        exit(1);
    }

    k_sockinfo *SM = (k_sockinfo *)shmat(shmid, NULL, 0);
    if (SM == (void *)-1)
    {
        perror("initsocket: shmat");
        exit(1);
    }

    for (int i = 0; i < N; i++)
    {
        SM[i].is_free = true;
        for (int j = 0; j < BUFFSIZE; j++)
            SM[i].send_buff[j] = NULL;
        SM[i].recv_buff = init_queue();
        bzero(&SM[i].dest_addr, sizeof(SM[i].dest_addr));
    }

    printf("Shared memory initialized with ID: %d\n", shmid);
    shmdt(SM);
}

void initk_sem()
{
    key_t key = ftok(SEM_PATH, SEM_ID);
    int semid = semget(key, N, IPC_CREAT | 0777 | IPC_EXCL);
    if (semid == -1)
    {
        perror("initsocket: semget");
        exit(1);
    }

    printf("Semaphore initialized with ID: %d\n", semid);

    union semun arg;
    unsigned short vals[N];

    for (int i = 0; i < N; i++)
        vals[i] = 1;

    arg.array = vals;
    if (semctl(semid, 0, SETALL, arg) == -1)
    {
        perror("initsocket: semctl");
        exit(1);
    }
}

int main()
{
    initk_shm();
    initk_sem();
}