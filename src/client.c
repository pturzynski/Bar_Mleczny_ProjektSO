#include "ipc.h"

void handle_signal(int sig) {
    semunlock(SEM_GENERATOR);
    detach_ipc();
    exit(0);
}

int main(){
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    BarState *bar = join_ipc();
    msgbuf msg;
    srand(time(NULL) ^ getpid() << 16);
    int groupSize = (rand() % 3 + 1);
    semlock(SEM_MEMORY);
    bar->clients += groupSize;
    semunlock(SEM_MEMORY);

    int ifOrder = rand() % 101;
    if(ifOrder <= 5){
        semlock(SEM_MEMORY);
        bar->clients -= groupSize;
        semunlock(SEM_MEMORY);
        printf(CLIENT_COL "[KLIENT %d] Rezygnujemy z zamowienia. Wychodzimy z baru!\n" RESET, getpid());
        semunlock(SEM_GENERATOR);
        detach_ipc();  
        return 0;
    }
    msg.mtype = MTYPE_CASHIER;
    msg.groupSize = groupSize;
    msg.pid = getpid();
    semlock(SEM_CASHIER);
    semlock(SEM_MSG_LIMIT);
    msgSend(&msg);
    printf(CLIENT_COL "[KLIENT %d] Place\n" RESET, getpid());
    msgReceive(&msg, getpid());
    semunlock(SEM_CASHIER);
    semunlock(SEM_MSG_LIMIT);
    int myTable = msg.tableId;

    if(msg.order == 1){
        msg.mtype = MTYPE_WORKER;
        msg.pid = getpid();
        msg.action = WORKER_FOOD;
        semlock(SEM_WORKER);
        semlock(SEM_MSG_LIMIT);
        msgSend(&msg);
        msgReceive(&msg, getpid());
        semunlock(SEM_WORKER);
        semunlock(SEM_MSG_LIMIT);
        printf(CLIENT_COL "[KLIENT %d] Odebralem danie, jem\n" RESET, getpid());
        //sleep(10);

        msg.mtype = MTYPE_WORKER;
        msg.pid = getpid();
        msg.groupSize = groupSize;
        msg.tableId = myTable;
        msg.action = WORKER_CLEAN;
        semlock(SEM_MSG_LIMIT);
        semlock(SEM_WORKER);
        msgSend(&msg);
        semunlock(SEM_WORKER);
    }
    else if(msg.order == 0){
        semlock(SEM_MEMORY);
        bar->clients -= groupSize;
        semunlock(SEM_MEMORY);
        printf(CLIENT_COL "[KLIENT %d] Opuszczamy bar\n" RESET, getpid());
        semunlock(SEM_GENERATOR);
        detach_ipc();  
        return 0;
    }

    semlock(SEM_MEMORY);
    bar->clients -= groupSize;
    semunlock(SEM_MEMORY);
    printf(CLIENT_COL "[KLIENT %d] Opuszczamy bar\n" RESET, getpid());
    semunlock(SEM_GENERATOR);
    detach_ipc();  
    return 0;
}