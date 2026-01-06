#include "ipc.h"


int main(){
    BarState *bar = join_ipc();
    msgbuf msg;

    srand(time(NULL) ^ getpid() << 16);
    int groupSize = (rand() % 3 + 1);
    semlock(SEM_MEMORY);
    bar->clients += groupSize;
    semunlock(SEM_MEMORY);
    printf("[KLIENT %d] Jestesmy grupa %d osob!\n", getpid(), groupSize);
    int ifOrder = rand() % 101;
    if (ifOrder <= 5){
        semlock(SEM_MEMORY);
        bar->clients -= groupSize;
        semunlock(SEM_MEMORY);
        printf("[KLIENT %d] Rezygnujemy z zamowienia. Wychodzimy z baru!\n", getpid());
        detach_ipc();  
        return 0;
    }
    msg.mtype = MTYPE_CASHIER;
    msg.pid = getpid();
    msg.groupSize = groupSize;
    semlock(SEM_CASHIER);
    msgSend(&msg);
    printf("[KLIENT %d] Zamawiamy!\n", getpid());
    semunlock(SEM_CASHIER);
    sleep(10);
    semlock(SEM_MEMORY);
    printf("[KLIENT %d] Grupa %d osob wychodzi z baru!\n", getpid(), groupSize);
    bar->clients -= groupSize;
    semunlock(SEM_MEMORY);
    detach_ipc();  
    return 0;
}