#include "ipc.h"

static BarState *bar = NULL;
static int shmid = -1;
static int semid = -1;
static int msgid = -1;
static const int msgSize = sizeof(msgbuf) - sizeof(long int);

BarState* init_ipc(int x1, int x2, int x3, int x4, int maxTables){
    key_t key = ftok(FTOK_PATH, FTOK_KEY);
    if (key == -1){
        perror("ftok error");
        exit(1);
    }

    shmid = shmget(key, sizeof(BarState) + (maxTables * sizeof(Table)), IPC_CREAT | 0600);
    if (shmid == -1){
        perror("shmget error (init)");
        exit(1);
    }

    semid = semget(key, SEMNUMBER, IPC_CREAT | 0600);
    if (semid == -1){
        perror("semget error (init)");
        exit(1);
    }

    msgid = msgget(key, IPC_CREAT | 0600);
    if(msgid == -1){
        perror("msgget error (init)");
        exit(1);
    }

    bar = (BarState*)shmat(shmid, NULL, 0);
    if (bar == (void*) - 1){
        perror("shmat error (init)");
        exit(1);
    }

    for (int i = 0; i < SEMNUMBER; i++){
        if (semctl(semid, i, SETVAL, 1) == -1) {
            perror("semctl SETVAL error (init)");
            exit(1);
        }
    }

    memset(bar, 0, sizeof(BarState) + (maxTables * sizeof(Table)));
    bar->x1 = x1;
    bar->x2 = x2;
    bar->x3 = x3;
    bar->x4 = x4;
    bar->flagReservation = 0;
    bar->flagDoubleX3 = 0;
    bar->flagFire = 0;
    bar->clients = 0;
    bar->maxClients = 1*x1 + 2*x2 + 3*x3 + 4*x4;
    int ind = 0;
    for(int i = 0; i<x1; i++){
        bar->tables[ind].id = ind;
        bar->tables[ind].capacity = 1;
        bar->tables[ind].groupSize = 0;
        bar->tables[ind].freeSlots = 1;
        bar->tables[ind].isReserved = 0;
        ind++;
    }

    for(int i = 0; i<x2; i++){
        bar->tables[ind].id = ind;
        bar->tables[ind].capacity = 2;
        bar->tables[ind].groupSize = 0;
        bar->tables[ind].freeSlots = 2;
        bar->tables[ind].isReserved = 0;
        ind++;
    }

    for(int i = 0; i<x3; i++){
        bar->tables[ind].id = ind;
        bar->tables[ind].capacity = 3;
        bar->tables[ind].groupSize = 0;
        bar->tables[ind].freeSlots = 3;
        bar->tables[ind].isReserved = 0;
        ind++;
    }

    for(int i = 0; i<x4; i++){
        bar->tables[ind].id = ind;
        bar->tables[ind].capacity = 4;
        bar->tables[ind].groupSize = 0;
        bar->tables[ind].freeSlots = 4;
        bar->tables[ind].isReserved = 0;
        ind++;
    }
    
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
    shmid = shmget(key, 0, 0600);
    if (shmid == -1){
        perror("shmget join error");
    }

    semid = semget(key, SEMNUMBER, 0600);
    if (semid == -1) { 
        perror("semget join error"); 
        exit(1); 
    }
    msgid = msgget(key, 0600);
    if (msgid == -1) {
        perror("msgget join error");
        exit(1); 
    }

    bar = (BarState*)shmat(shmid, NULL, 0);
    if (bar == (void*) - 1){
        perror("shmat error");
        exit(1);
    }

    return bar;
}

void detach_ipc() {
    if (bar != NULL) {
        int dt = shmdt(bar);
        if (dt == -1){
            perror("shmdt error (detach)");
        }
        bar = NULL;
    }
}

void cleanup_ipc(){
    if ((shmctl(shmid, IPC_RMID, NULL)) == -1){
        perror("shmctl IPIC_RMID error");
    }
    
    if (semctl(semid, 0, IPC_RMID) == -1){ 
        perror("semctl IPC_RMID error"); 
    }

    if (msgctl(msgid, IPC_RMID, NULL) == -1){
        perror("msgctl IPC_RMID error");
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

void msgSend(msgbuf *m){
    if(msgsnd(msgid, m, msgSize, 0) == -1){
        perror("Message send error");
        exit(1);
    }
}

int msgReceive(msgbuf *m, long int mtype){
    int result = msgrcv(msgid, m, msgSize, mtype, 0);
    if (result == -1){
        perror("Message receive error");
        exit(1);
    }
    return result;
}