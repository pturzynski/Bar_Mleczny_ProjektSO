#include "include/ipc.h"

BarState *bar = NULL;

int shmid = -1;
int semid = -1;

int msgClient = -1;
int msgCashier = -1;
int msgWorker = -1; 
int msgStaff = -1;

static const int msgSize = sizeof(msgbuf) - sizeof(long int);

key_t getKey(char id){
    const char* path = "keyfile";

    int file = open(path, O_CREAT | O_RDWR, 0600);
    if (file == -1) {
        perror("open keyfile error");
        exit(1);
    }
    close(file);

    key_t key = ftok(path, id);
    if(key == -1){
        perror("ftok error");
        exit(1);
    }
    return key;
}

//pamiec wspoldzieona
BarState* init_shmem(int x1, int x2, int x3, int x4, int maxTables){
    key_t key = getKey(KEY_SHMEM);

    shmid = shmget(key, sizeof(BarState) + (maxTables * sizeof(Table)), IPC_CREAT | 0600);
    if(shmid == -1){
        perror("shmget error (init)");
        exit(1);
    }

    bar = (BarState*)shmat(shmid, NULL, 0);
    if(bar == (void*) - 1){
        perror("shmat error (init)");
        exit(1);
    }

    memset(bar, 0, sizeof(BarState) + (maxTables * sizeof(Table)));
    bar->x1 = x1;
    bar->x2 = x2;
    bar->x3 = x3;
    bar->x4 = x4;
    bar->allTables = x1 + x2 + x3 + x4;
    bar->maxTables = maxTables;
    bar->flagReservation = 0;
    bar->flagDoubleX3 = 0;
    bar->flagFire = 0;
    bar->clients = 0;
    bar->maxClients = 1*x1 + 2*x2 + 3*x3 + 4*x4;

    int ind = 0;
    for(int i = 0; i<x1; i++){
        bar->tables[ind].id = ind;
        bar->tables[ind].capacity = 1;
        bar->tables[ind].whoSits = 0;
        bar->tables[ind].freeSlots = 1;
        bar->tables[ind].isReserved = 0;
        ind++;
    }

    for(int i = 0; i<x2; i++){
        bar->tables[ind].id = ind;
        bar->tables[ind].capacity = 2;
        bar->tables[ind].whoSits = 0;
        bar->tables[ind].freeSlots = 2;
        bar->tables[ind].isReserved = 0;
        ind++;
    }

    for(int i = 0; i<x3; i++){
        bar->tables[ind].id = ind;
        bar->tables[ind].capacity = 3;
        bar->tables[ind].whoSits = 0;
        bar->tables[ind].freeSlots = 3;
        bar->tables[ind].isReserved = 0;
        ind++;
    }

    for(int i = 0; i<x4; i++){
        bar->tables[ind].id = ind;
        bar->tables[ind].capacity = 4;
        bar->tables[ind].whoSits = 0;
        bar->tables[ind].freeSlots = 4;
        bar->tables[ind].isReserved = 0;
        ind++;
    }

    return bar;
}

//semafory
int init_semaphores(int max_clients){
    key_t key = getKey(KEY_SEM);

    semid = semget(key, SEMNUMBER, IPC_CREAT | 0600);
    if(semid == -1){
        perror("semget error (init)");
        exit(1);
    }

    for(int i = 0; i < SEMNUMBER; i++){
        int value = 1;

        if(i == SEM_GENERATOR){
            value = MAX_PROCESSES;
        }

        if(i == SEM_DOOR){
            value = bar->maxClients;
        }

        if(i == SEM_SEARCH){
            value = 0;
        }

        if(semctl(semid, i, SETVAL, value) == -1) {
            perror("semctl SETVAL error (init)");
            exit(1);
        }
    }

    return semid;
}

//kolejka
void init_queue(){
    msgClient = msgget(getKey('A'), IPC_CREAT | 0600);
    msgCashier = msgget(getKey('B'), IPC_CREAT | 0600);
    msgWorker = msgget(getKey('C'), IPC_CREAT | 0600);
    msgStaff = msgget(getKey('D'), IPC_CREAT | 0600);

    if(msgClient == -1 || msgCashier == -1 || msgWorker == -1 || msgStaff == -1){
        perror("error msgget");
        exit(1);
    }
}

BarState* init_ipc(int x1, int x2, int x3, int x4, int maxTables){
    BarState *bar = init_shmem(x1, x2, x3, x4, maxTables);
    semid = init_semaphores(bar->maxClients);
    init_queue();
    return bar;
}

BarState* join_ipc(){
    if(bar != NULL){
        return bar;
    }

    key_t key_shmem = getKey(KEY_SHMEM);
    key_t key_sem = getKey(KEY_SEM);

    shmid = shmget(key_shmem, 0, 0600);
    if(shmid == -1){
        perror("shmget join error");
        exit(1);
    }

    bar = (BarState*)shmat(shmid, NULL, 0);
    if(bar == (void*) - 1){
        perror("shmat join error");
        exit(1);
    }

    semid = semget(key_sem, SEMNUMBER, 0600);
    if(semid == -1) { 
        perror("semget join error"); 
        exit(1); 
    }

    msgClient = msgget(getKey('A'), 0600);
    msgCashier = msgget(getKey('B'), 0600);
    msgWorker = msgget(getKey('C'), 0600);
    msgStaff = msgget(getKey('D'), 0600);

    if (msgClient == -1 || msgCashier == -1 || msgWorker == -1 || msgStaff == -1) {
        perror("msgget join error");
        exit(1);
    }

    return bar;
}

void detach_ipc() {
    if(bar != NULL) {
        int dt = shmdt(bar);
        if(dt == -1){
            perror("shmdt error (detach)");
        }
        bar = NULL;
    }
}

void cleanup_ipc() {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("cleanup shared memory error");
    } else {
        shmid = -1;
    }

    if(semctl(semid, 0, IPC_RMID) == -1) {
        perror("cleanup semaphore error");
    } else {
        semid = -1;
    }

    if(msgctl(msgClient, IPC_RMID, NULL) == -1) {
        perror("cleanup msgClient error");
    }
    if(msgctl(msgCashier, IPC_RMID, NULL) == -1) {
        perror("cleanup msgCashier error");
    }
    if(msgctl(msgWorker, IPC_RMID, NULL) == -1) {
        perror("cleanup msgWorker error");
    }
    if(msgctl(msgStaff, IPC_RMID, NULL) == -1) {
        perror("cleanup msgStaff error");
    }
    msgClient = msgCashier = msgWorker = msgStaff = -1;
}

void semlock(int sem_num){
    struct sembuf op; //p
    op.sem_num = sem_num;
    op.sem_op = -1;
    op.sem_flg = 0;
    while (1){
        if(semop(semid, &op, 1) == 0){
            break;
        }
        if(errno == EINTR){
            continue;
        } 
        if(errno == EIDRM || errno == EINVAL){
            break;
        }
        else{
            perror("semop lock error");
            exit(1);
        }
    }
}

void semunlock(int sem_num){
    struct sembuf op; //v
    op.sem_num = sem_num;
    op.sem_op = 1;
    op.sem_flg = 0;
    while (1) {
        if(semop(semid, &op, 1) == 0){
            break;
        }
        if(errno == EINTR){
            continue;
        } 
        if(errno == EIDRM || errno == EINVAL){
            break;
        }
        else{
            perror("semop unlock error");
            exit(1);
        }
    }
}

void sem_closeDoor(int sem_num, int groupSize){
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = -groupSize;
    op.sem_flg = 0;
    while (1) {
        if(semop(semid, &op, 1) == 0){
            break;
        }
        if(errno == EINTR){
            continue;
        } 
        if(errno == EIDRM || errno == EINVAL){
            break;
        }
        else{
            perror("semop door lock error");
            exit(1);
        }
    }
}

void sem_openDoor(int sem_num, int groupSize){
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = groupSize;
    op.sem_flg = 0;
    while (1) {
        if(semop(semid, &op, 1) == 0){
            break;
        }
        if(errno == EINTR){
            continue;
        } 
        if(errno == EIDRM || errno == EINVAL){
            break;
        }
        else{
            perror("semop door lock error");
            exit(1);
        }
    }
}

void sem_wakeWaiting(int sem_num){
    int waiting = semctl(semid, sem_num, GETNCNT); //getncnt - ile klientow spi na semafroze
    if(waiting <= 0){
        return;
    }
    for(int i = 0; i < waiting; i++){
        semunlock(SEM_SEARCH);
    }
}

void msgSend(int dest, msgbuf *msg){
    while(1){
        if (msgsnd(dest, msg, msgSize, 0) == -1){
            if(errno == EINTR){
                continue;
            }
            if(errno == EIDRM || errno == EINVAL){
                return;
            }
            perror("msgsnd error");
            exit(1);
        }
        break;
    }
}

int msgReceive(int src, msgbuf *msg, long type, int nowait){
    int flag = 0;
    if (nowait == 1) {
        flag = IPC_NOWAIT;
    }

    int result;
    while(1){
        result = msgrcv(src, msg, msgSize, type, flag);
        if (result == -1) {
            if(errno == EINTR){
                continue;
            } 
            if(errno == EIDRM){
                break;
            }
            else{
                perror("msgrcv error");
                exit(1);
            }
        }
        break;
    }
    return result;
}