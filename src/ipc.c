#include "ipc.h"

static BarState *bar;
int semid = -1;

BarState* init_ipc(int x1, int x2, int x3, int x4){
    key_t key = ftok(FTOK_PATH, FTOK_KEY);
    if (key == -1){
        perror("ftok error");
        exit(1);
    }

    int shmid = shmget(key, sizeof(BarState), IPC_CREAT | 0666);
    if (shmid == -1){
        perror("shmget error");
        exit(1);
    }

    bar = (BarState*)shmat(shmid, NULL, 0);
    if (bar == (void*) - 1){
        perror("shmat error");
        exit(1);
    }

    int semnumber = 5;
    semid = semget(key, semnumber, IPC_CREAT | 0666);
    if (semid == -1){
        perror("semget error");
        exit(1);
    }
    for (int i = 0; i < semnumber; i++){
        if (semctl(semid, i, SETVAL, 1) == -1) {
            perror("semctl SETVAL error");
            exit(1);
        }
    }

    memset(bar, 0, sizeof(BarState));
    bar->x1 = x1;
    bar->freeX1 = x1;
    bar->x2 = x2;
    bar->freeX2 = x2;
    bar->x3 = x3;
    bar->freeX3 = x3;
    bar->x4 = x4;
    bar->freeX4 = x4;
    bar->flagDoubleX3 = 0;
    bar->flagFire = 0;
    bar->clients = 0;
    bar->maxClients = 1*x1 + 2*x2 + 3*x3 + 4*x4;

    return bar;
}

BarState* join_ipc(){
    if (bar != NULL){
        return bar;
    }
    key_t key = ftok(FTOK_PATH, FTOK_KEY);
    if (key == -1){
        perror("ftok error");
        exit(1);
    }

    int shmid = shmget(key, sizeof(BarState), 0666);
    if (shmid == -1){
        perror("shmget error");
        exit(1);
    }

    bar = (BarState*)shmat(shmid, NULL, 0);
    if (bar == (void*) - 1){
        perror("shmat error");
        exit(1);
    }

    semid = semget(key, 1, 0666);
    if (semid == -1) { 
        perror("semget error"); 
        exit(1); 
    }
    return bar;
}

void detach_ipc() {
    if (bar != NULL) {
        shmdt(bar);
        bar = NULL;
    }
}

void cleanup_ipc(){
    key_t key = ftok(FTOK_PATH, FTOK_KEY);
    if (key == -1){
        perror("ftok error");
        exit(1);
    }

    int shmid = shmget(key, sizeof(BarState), 0666);
    if (shmid == -1){
        perror("shmget error");
        exit(1);
    }

    int clear = shmctl(shmid, IPC_RMID, NULL);
    if (clear == -1){
        perror("shmctl error");
        exit(1);
    }
    
    if (semctl(semid, 0, IPC_RMID) == -1){ 
        perror("semctl IPC_RMID error"); 
        exit(1); 
    }
}

void semlock(int sem_num){
    struct sembuf operation; //p
    operation.sem_num = sem_num;
    operation.sem_op = -1;
    operation.sem_flg = 0;
    if (semop(semid, &operation, 1) == -1){     
        perror("sem lock error"); 
        exit(1); 
    }
}

void semunlock(int sem_num){
    struct sembuf operation; //v
    operation.sem_num = sem_num;
    operation.sem_op = 1;
    operation.sem_flg = 0;
    if (semop(semid, &operation, 1) == -1){ 
        perror("sem unlock error"); 
        exit(1); 
    }
}