#include "include/ipc.h"

BarState *bar = NULL;
static const int msgSize = sizeof(msgbuf) - sizeof(long int);

int shmid = -1;
int semid = -1;

int msgOrder = -1;
int msgFood = -1;
int msgStaff = -1;

int loggerFile = -1;

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

    shmid = shmget(key, sizeof(BarState) + (maxTables * sizeof(Table)), IPC_CREAT | IPC_EXCL | 0600);
    if(shmid == -1){
        if(errno == EEXIST){
            fprintf(stderr, "Blad, symulacja juz dziala w innym oknie\n");
        }
        else{
            perror("shmget error (init)");
        }
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

    semid = semget(key, SEMNUMBER, IPC_CREAT | IPC_EXCL | 0600);
    if(semid == -1){
        if(errno == EEXIST){
            fprintf(stderr, "Blad, semafory juz istnieja\n");
        }
        else{
            perror("semget error (init)");
        }
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
        if(i == SEM_ORDER){
            value = 100;
        }
        if(i == SEM_FOOD){
            value = 100;
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
    msgOrder = msgget(getKey('A'), IPC_CREAT | IPC_EXCL | 0600);
    msgFood = msgget(getKey('B'), IPC_CREAT | IPC_EXCL |0600);
    msgStaff = msgget(getKey('C'), IPC_CREAT | IPC_EXCL | 0600);

    if(msgOrder == -1 || msgFood == -1 || msgStaff == -1){
        if(errno == EEXIST){
            fprintf(stderr, "Blad, kolejki komunikatow juz istnieja\n");
        }
        else{
            perror("error msgget");
        }
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
        if(errno == ENOENT){
            fprintf(stderr, "Bar zamkniety, ./main nie jest uruchomiony\n");
        }
        else{
            perror("shmget join error");
        }
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

    msgOrder = msgget(getKey('A'), 0600);
    msgFood = msgget(getKey('B'), 0600);
    msgStaff = msgget(getKey('C'), 0600);

    if (msgOrder == -1 || msgFood == -1 || msgStaff == -1){
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

    if(msgctl(msgOrder, IPC_RMID, NULL) == -1) {
        perror("cleanup msgOrder error");
    }
    if(msgctl(msgFood, IPC_RMID, NULL) == -1) {
        perror("cleanup msgFood error");
    }
    if(msgctl(msgStaff, IPC_RMID, NULL) == -1){
        perror("cleanup msgStaff error");
    }
    msgOrder = msgFood = msgStaff = -1;
}

int semlock(int sem_num, int undo){
    struct sembuf op; //p
    op.sem_num = sem_num;
    op.sem_op = -1;
    if(undo == 1){
        op.sem_flg = SEM_UNDO;
    }
    else{
        op.sem_flg = 0;
    }

    while (semop(semid, &op, 1) == -1) {
        if (errno == EINTR) {
            return -1;
        }
        if (errno == EIDRM || errno == EINVAL) {
            return -2;
        }
        perror("semunlock error");
        exit(1);
    }
    return 0;
}


int semunlock(int sem_num, int undo){
    struct sembuf op; //v
    op.sem_num = sem_num;
    op.sem_op = 1;
    if(undo == 1){
        op.sem_flg = SEM_UNDO;
    } 
    else{
        op.sem_flg = 0;
    }
    while (1) {
        if (semop(semid, &op, 1) == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EIDRM || errno == EINVAL) {
            return -2;
        }
        perror("semunlock error");
        exit(1);
    }
    return 0;
}

int sem_closeDoor(int sem_num, int groupSize, int undo){
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = -groupSize;
    if(undo == 1){
        op.sem_flg = SEM_UNDO;
    } 
    else{
        op.sem_flg = 0;
    }
    while (1) {
        if (semop(semid, &op, 1) == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EIDRM || errno == EINVAL) {
            return -2;
        }
        perror("sem closeDoor error");
        exit(1);
    }
    return 0;
}

int sem_openDoor(int sem_num, int groupSize, int undo){
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = groupSize;
    if(undo == 1){
        op.sem_flg = SEM_UNDO;
    } 
    else{
        op.sem_flg = 0;
    }
    while (1) {
        if (semop(semid, &op, 1) == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EIDRM || errno == EINVAL) {
            return -2;
        }
        perror("sem openDoor error");
        exit(1);
    }
    return 0;
}

int sem_wakeOne(int sem_num){
    int waiting = semctl(semid, sem_num, GETNCNT); //getncnt - ile klientow spi na semafroze
    if(waiting <= 0){
        return 0;
    }
    semunlock(sem_num, 0);
    return 0;
}

int msgSend(int dest, msgbuf *msg){
    while (1) {
        if (msgsnd(dest, msg, msgSize, 0) == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EIDRM || errno == EINVAL) {
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

void loggerOpen(){
    loggerFile = open(LOG_FILE, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if(loggerFile == -1){
        perror("Błąd otwarcia pliku logów w logger_open");
    }
}

void loggerClose(){
    if(loggerFile != -1){
        close(loggerFile);
        loggerFile = -1;
    }
}

void logger(const char *format, ...) {
    char buffer[4096];
    va_list args;

    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if(len <= 0){
        return;
    }

    if(len >= (int)sizeof(buffer)){
        len = sizeof(buffer) - 1;
    }
    
    if(len > 0 && buffer[len-1] != '\n') {
        if(len < (int)sizeof(buffer) - 1) {
            buffer[len++] = '\n';
        } 
        else {
            buffer[len-1] = '\n';
        }
    }
    buffer[len] = '\0';

    write(STDOUT_FILENO, buffer, len);

    if(loggerFile != -1){
        char clean_buffer[4096];
        char *src = buffer;
        char *dst = clean_buffer;
        char *end = buffer + len; 
        
        while(src < end) {
            if(*src == '\033') {
                while(src < end && *src != 'm') {
                    src++;
                }
                if(src < end) src++;
            } 
            else {
                *dst++ = *src++;
            }
        }
        
        int clean_len = dst - clean_buffer;
        
        flock(loggerFile, LOCK_EX); 
        write(loggerFile, clean_buffer, clean_len);
        flock(loggerFile, LOCK_UN);
    }
}