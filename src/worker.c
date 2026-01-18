#include "include/ipc.h"

volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t doubleX3 = 0;

void handle_signal(int sig) {
    if(sig == SIGTERM || sig == SIGINT){
        keep_running = 0;
    }
    if(sig == SIGUSR1){
        doubleX3 = 1;
    }
}

int main(){
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    signal(SIGUSR1, handle_signal);

    BarState *bar = join_ipc();
    bar->workerPid = getpid();
    msgbuf msg;
    printf(WORKER_COL "[PRACOWNIK] Rozpoczynam prace!\n" RESET);

    while(keep_running){
        if(doubleX3 == 1){
            doubleX3 = 0;
            
            semlock(SEM_MEMORY);
            
            if(bar->flagDoubleX3 == 1){
                semunlock(SEM_MEMORY);
                printf(WORKER_COL "[PRACOWNIK] Stoliki juz zostaly podwojone\n" RESET);
                msg.mtype = MTYPE_STAFF;
                msg.success = 0;
                msgSend(msgStaff, &msg);
                continue;
            }
            
            int x3 = bar->x3;
            int newTables = bar->allTables + x3;
            if(newTables > bar->maxTables){
                semunlock(SEM_MEMORY);
                printf(WORKER_COL "[PRACOWNIK] Nie mozna podwoic stolikow\n" RESET);
                msg.mtype = MTYPE_STAFF;
                msg.success = 0;
                msgSend(msgStaff, &msg);
                continue;
            }

            int ind = bar->allTables;
            for(int i = 0; i < x3; i++){
                bar->tables[ind].id = ind;
                bar->tables[ind].capacity = 3;
                bar->tables[ind].whoSits = 0;
                bar->tables[ind].freeSlots = 3;
                bar->tables[ind].isReserved = 0;
                ind++;
            }
            bar->x3 += x3;
            bar->allTables = newTables;
            bar->maxClients += 3 * x3;
            bar->flagDoubleX3 = 1;
            
            semunlock(SEM_MEMORY);
            
            sem_openDoor(SEM_DOOR, 3 * x3);
            msg.mtype = MTYPE_STAFF;
            msg.success = 1;
            msgSend(msgStaff, &msg);
            continue;
        }
        if(msgReceive(msgWorker, &msg, MTYPE_WORKER, 0) != -1){
            printf(WORKER_COL "[PRACOWNIK] Wydaje zamowienie dla %d\n" RESET, msg.pid);
            msg.mtype = msg.pid;
            msgSend(msgClient, &msg);
        }
    }

    printf(WORKER_COL "[PRACOWNIK] Koncze prace!\n" RESET);
    detach_ipc();
    return 0;
}
