#include "include/ipc.h"

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    if(sig == SIGQUIT){
        printf(WORKER_COL "[PRACOWNIK] POZAR ! EWAKUACJA !\n" RESET);
        detach_ipc();
        exit(0);
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

    semlock(SEM_MEMORY);
    bar->workerPid = getpid();
    semunlock(SEM_MEMORY);

    printf(WORKER_COL "[PRACOWNIK] Rozpoczynam prace!\n" RESET);
    while(running){
        msgReceive(msgWorker, &msg, MTYPE_WORKER);
        printf(WORKER_COL "[PRACOWNIK] Wydaje zamowienie dla %d\n" RESET, msg.pid);
        msg.mtype = msg.pid;
        msgSend(msgClient, &msg);

    }

    printf(WORKER_COL "[PRACOWNIK] Koncze prace!\n" RESET);
    detach_ipc();
    return 0;
}
