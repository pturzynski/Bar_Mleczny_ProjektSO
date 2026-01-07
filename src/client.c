#include "ipc.h"

int main(){
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
        detach_ipc();  
        return 0;
    }
    printf(CLIENT_COL "[KLIENT %d] Zamawiam\n" RESET, getpid());
    msg.mtype = MTYPE_CASHIER;
    msg.groupSize = groupSize;
    msg.pid = getpid();
    semlock(SEM_CASHIER);
    msgSend(&msg);
    semunlock(SEM_CASHIER);
    printf(CLIENT_COL "[KLIENT %d] Place\n" RESET, getpid());
    msgReceive(&msg, getpid());
    int myTable = msg.tableId;

    if(msg.order == 1){
        msg.mtype = MTYPE_WORKER;
        msg.pid = getpid();
        msg.action = WORKER_FOOD; 
        msgSend(&msg);

        msgReceive(&msg, getpid());
        printf(CLIENT_COL "[KLIENT %d] Odebralem danie, jem\n" RESET, getpid());
        sleep(10);

        msg.mtype = MTYPE_WORKER;
        msg.pid = getpid();
        msg.groupSize = groupSize;
        msg.tableId = myTable;
        msg.action = WORKER_CLEAN;
        msgSend(&msg);
    }

    else{
        printf(CLIENT_COL "[KLIENT %d] Brak miejsc\n" RESET, getpid());
        sleep(2);
    }

    semlock(SEM_MEMORY);
    bar->clients -= groupSize;
    semunlock(SEM_MEMORY);
    printf(CLIENT_COL "[KLIENT %d] Opuszczamy bar\n" RESET, getpid());
    printf("Stan baru: %d\n", bar->clients);
    detach_ipc();  
    return 0;
}