#include "include/ipc.h"

volatile sig_atomic_t doubleX3 = 0;
int done = 0;
volatile sig_atomic_t reservation = 0;

void handle_signal(int sig){
    if(sig == SIGUSR1){
        doubleX3 = 1;
    }
    if(sig == SIGUSR2){
        reservation = 1;
    }
    if(sig == SIGINT){
        loggerClose();
        logger(WORKER_COL "[PRACOWNIK] Koncze prace!" RESET);
        _exit(0);
    }
    if(sig == SIGTERM){
        logger(WORKER_COL "[PRACOWNIK] Klienci ewakuowani, uciekam tez!" RESET);
        loggerClose();
        _exit(0);
    }
}

int main(){
    loggerOpen();
    bar = join_ipc();
    msgbuf msg;

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    semlock(SEM_MEMORY, 1);
    bar->workerPid = getpid();
    semunlock(SEM_MEMORY, 1);

    logger(WORKER_COL "[PRACOWNIK] Rozpoczynam prace!" RESET);
    int running = 1;
    while(running){
        if(doubleX3 == 1 && done == 0){
            semlock(SEM_MEMORY, 1);
            int newTables = bar->allTables + bar->x3;
            if(newTables > bar->maxTables){
                logger(WORKER_COL "[PRACOWNIK] Nie udalo sie podwoic stolikow x3" RESET);
                semunlock(SEM_MEMORY, 1);
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
                sem_openDoor(SEM_DOOR, 3 * oldX3, 0);
                if(bar->managerPid > 0){
                    kill(bar->managerPid, SIGUSR1);
                }
                semunlock(SEM_MEMORY, 1);
                done = 1;
                logger(WORKER_COL "[PRACOWNIK] Podwojono stoliki 3 osobowe: bylo: %d teraz = %d allTables = %d" RESET, oldX3, bar->x3, bar->allTables);
            }
        }
        else if(reservation == 1){
            int res = msgReceive(msgStaff, &msg, MTYPE_RESERVATION);
            if(res == -1){
                reservation = 0;
                continue;
            }
            if(res == -2){
                reservation = 0;
                break;
            }

            semlock(SEM_MEMORY, 1);

            int available = 0;
            for(int i = 0; i < bar->allTables; i++){
                if(bar->tables[i].capacity == msg.tableType && bar->tables[i].isReserved == 0){
                    available++;
                }
            }

            if(available >= msg.count){
                int reserved = 0;
                for(int i = 0; i < bar->maxTables && reserved < msg.count; i++){
                    if(msg.tableType == bar->tables[i].capacity && bar->tables[i].isReserved == 0){
                        bar->tables[i].isReserved = 1;
                        reserved++;
                    }
                }
                
                semunlock(SEM_MEMORY, 1);
                logger(WORKER_COL "[PRACOWNIK] Zarezerwowalem %d stolikow %d osobwych" RESET, reserved, msg.tableType);
                kill(bar->managerPid, SIGUSR1);
            }
            else{
                semunlock(SEM_MEMORY, 1);
                logger(WORKER_COL "[PRACOWNIK] Nie udalo sie zarezerwowac stolikow" RESET);
                kill(bar->managerPid, SIGUSR2);
            }
            reservation = 0;
        }
        int res = msgReceive(msgFood, &msg, MTYPE_WORKER);
        if(res == -1){
            continue;
        }
        if(res == -2){
            break;
        }

        logger(WORKER_COL "[PRACOWNIK] Wydaje zamowienie dla %d" RESET, msg.pid);
        msg.mtype = msg.pid;
        msgSend(msgFood, &msg);
    }

    logger(WORKER_COL "[PRACOWNIK] Koncze prace!" RESET);
    loggerClose();
    detach_ipc();
    return 0;
}
