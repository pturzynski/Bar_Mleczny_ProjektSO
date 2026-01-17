#include "include/ipc.h"

int keep_running = 1;

void handle_signal(int sig) {
    keep_running = 0;
}

int main(){
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    BarState *bar = join_ipc();
    msgbuf msg;
    printf(WORKER_COL "[PRACOWNIK] Rozpoczynam prace!\n" RESET);

    while(keep_running){
        msgReceive(msgWorker, &msg, MTYPE_WORKER, 0);
        printf(WORKER_COL "[PRACOWNIK] Wydaje zamowienie dla %d\n" RESET, msg.pid);
        msg.mtype = msg.pid;
        msgSend(msgClient, &msg);
    }
    printf(WORKER_COL "[PRACOWNIK] Koncze prace!\n" RESET);
    detach_ipc();
    return 0;
}
