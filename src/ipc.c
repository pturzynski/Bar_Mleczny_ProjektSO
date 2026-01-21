#include "include/ipc.h"

BarState *bar = NULL;

int shmid = -1;
int semid = -1;

int msgClient = -1;
int msgCashier = -1;
int msgWorker = -1; 

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
    bar->clients = 0;
    bar->workerPid = 0;
    bar->mainPid = 0;
    bar->managerPid = 0;
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
int init_semaphores(){
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

    if(msgClient == -1 || msgCashier == -1 || msgWorker == -1){
        perror("error msgget");
        exit(1);
    }
}

BarState* init_ipc(int x1, int x2, int x3, int x4, int maxTables){
    BarState *bar = init_shmem(x1, x2, x3, x4, maxTables); 
    semid = init_semaphores();
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

    if (msgClient == -1 || msgCashier == -1 || msgWorker == -1) {
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
    msgClient = msgCashier = msgWorker = -1;
}

int semlock(int sem_num){
    struct sembuf op; //p
    op.sem_num = sem_num;
    op.sem_op = -1;
    op.sem_flg = 0;
    if(semop(semid, &op, 1) == -1){
        if(errno == EINTR){
            return -1;
        }
        if(errno == EIDRM || errno == EINVAL){
            return -2;
        } 
        perror("semlock error");
        exit(1);
    }
    return 0;
}

int semunlock(int sem_num){
    struct sembuf op; //v
    op.sem_num = sem_num;
    op.sem_op = 1;
    op.sem_flg = 0;
    if(semop(semid, &op, 1) == -1){
        if(errno == EINTR){
            return -1;
        }
        if(errno == EIDRM || errno == EINVAL){
            return -2;
        }
        perror("semunlock error");
        exit(1);
    }
    return 0;
}

int sem_closeDoor(int sem_num, int groupSize){
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = -groupSize;
    op.sem_flg = 0;
    if(semop(semid, &op, 1) == -1){
        if(errno == EINTR){
            return -1;
        }
        if(errno == EIDRM || errno == EINVAL){
            return -2;
        }
        perror("semop door lock error");
        exit(1);
    }
    return 0;
}

int sem_openDoor(int sem_num, int groupSize){
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = groupSize;
    op.sem_flg = 0;
    if(semop(semid, &op, 1) == -1){
        if(errno == EINTR){
            return -1;
        }
        if(errno == EIDRM || errno == EINVAL){
            return -2;
        }
        perror("semop door unlock error");
        exit(1);
    }
    return 0;
}

int sem_wakeAll(int sem_num){
    int waiting = semctl(semid, sem_num, GETNCNT); //getncnt - ile klientow spi na semafroze
    if(waiting <= 0){
        return 0;
    }
    for(int i = 0; i < waiting; i++){
        semunlock(sem_num);
    }
}

int msgSend(int dest, msgbuf *msg){
    if(msgsnd(dest, msg, msgSize, 0) == -1){
        if(errno == EINTR){
            return -1;
        }
        if(errno == EIDRM || errno == EINVAL){
            return -2;
        }
        perror("msgsnd error");
        exit(1);
    }
    return 0;
}

int msgReceive(int src, msgbuf *msg, long type){
    int result = msgrcv(src, msg, msgSize, type, 0);
    if (result == -1) {
        if(errno == EINTR){
            return -1;
        } 
        if(errno == EIDRM || errno == EINVAL){
            return -2;
        }
        perror("msgrcv error");
        exit(1);
    }
    return result;
}

void logger(const char *format, ...){
    va_list args1, args2;
    va_start(args1, format);
    va_copy(args2, args1);

    sigset_t block_mask, old_mask;
    sigfillset(&block_mask);
    sigprocmask(SIG_BLOCK, &block_mask, &old_mask);

    FILE *f = fopen(LOG_FILE, "a");
    if(f != NULL){
        int fd = fileno(f);
        flock(fd, LOCK_EX);

        vprintf(format, args1);
        printf("\n");
        fflush(stdout);

        vfprintf(f, format, args2);
        fprintf(f, "\n");
        fflush(f);
        
        flock(fd, LOCK_UN);
        fclose(f);
    }
    va_end(args1);
    va_end(args2);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}