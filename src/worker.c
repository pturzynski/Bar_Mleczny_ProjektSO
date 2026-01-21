#include "include/ipc.h"

volatile sig_atomic_t doubleX3 = 0;
int done = 0;

void handle_signal(int sig) {
    if(sig == SIGQUIT){
        logger(WORKER_COL "[PRACOWNIK] POZAR ! EWAKUACJA !" RESET);
        detach_ipc();
        exit(0);
    }
    if(sig == SIGINT){
        logger(WORKER_COL "[PRACOWNIK] Koncze prace!" RESET);
    }
    if(sig == SIGUSR1){
        doubleX3 = 1;
    }
}

int main(){
    BarState *bar = join_ipc();
    msgbuf msg;

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    semlock(SEM_MEMORY);
    bar->workerPid = getpid();
    semunlock(SEM_MEMORY);

    int running = 1;
    logger(WORKER_COL "[PRACOWNIK] Rozpoczynam prace!" RESET);
    while(running){
        if(doubleX3 == 1 && done == 0){
            semlock(SEM_MEMORY);
            int newTables = bar->allTables + bar->x3;
            if(newTables > bar->maxTables){
                logger(WORKER_COL "[PRACOWNIK] Nie udalo sie podwoic stolikow x3" RESET);
                semunlock(SEM_MEMORY);
                if(bar->managerPid > 0){
                    kill(bar->managerPid, SIGUSR2);
                }
            }
            else{
                int oldX3 = bar->x3;
                int ind = bar->allTables;
                for(int i = 0; i < bar->x3; i++){
                    bar->tables[ind].id = ind;
                    bar->tables[ind].capacity = 3;
                    bar->tables[ind].whoSits = 0;
                    bar->tables[ind].freeSlots = 3;
                    bar->tables[ind].isReserved = 0;
                    ind++;
                }
                bar->maxClients += 3 * oldX3;
                bar->x3 += oldX3;
                bar->allTables = newTables;
                sem_openDoor(SEM_DOOR, 3 * oldX3);
                if(bar->managerPid > 0){
                    kill(bar->managerPid, SIGUSR1);
                }
                semunlock(SEM_MEMORY);
                done = 1;
                logger(WORKER_COL "[PRACOWNIK] Podwojono stoliki 3 osobowe: bylo: %d teraz = %d allTables = %d" RESET, oldX3, bar->x3, bar->allTables);
            }
        }

        int res = msgReceive(msgWorker, &msg, MTYPE_WORKER);
        if(res == -1){
            continue;
        }
        if(res == -2){
            break;
        }

        logger(WORKER_COL "[PRACOWNIK] Wydaje zamowienie dla %d" RESET, msg.pid);
        msg.mtype = msg.pid;
        msgSend(msgClient, &msg);
    }

    printf(WORKER_COL "[PRACOWNIK] Koncze prace!" RESET);
    detach_ipc();
    return 0;
}
