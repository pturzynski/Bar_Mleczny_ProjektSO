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
    if (ifOrder <= 5){
        semlock(SEM_MEMORY);
        bar->clients -= groupSize;
        semunlock(SEM_MEMORY);
        printf(CLIENT_COL "[KLIENT %d] Rezygnujemy z zamowienia. Wychodzimy z baru!\n" RESET, getpid());
        detach_ipc();  
        return 0;
    }

    msg.mtype = MTYPE_CASHIER;
    msg.groupSize = groupSize;
    msg.pid = getpid();
    semlock(SEM_CASHIER);
    msgSend(&msg);
    semunlock(SEM_CASHIER);
    printf(CLIENT_COL "[KLIENT %d] Czekam na odpowiedz\n" RESET, getpid());
    msgReceive(&msg, getpid());
    if(msg.order == 1){
        printf(CLIENT_COL "[KLIENT %d] Jemy przy stoliku id: %d\n" RESET, getpid(), msg.tableId);
        sleep(10);
        printf(CLIENT_COL "[KLIENT %d] Zjedzone\n" RESET, getpid());
    }
    else{
        printf(CLIENT_COL "[KLIENT %d] Brak miejsc\n" RESET, getpid());
    }
    semlock(SEM_MEMORY);
    bar->clients -= groupSize;
    printf("STAN BARU %d OSOB\n", bar->clients);
    semunlock(SEM_MEMORY);
    detach_ipc();  
    return 0;
}