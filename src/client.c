#include "include/ipc.h"

int main(){
    BarState *bar = join_ipc();
    msgbuf msg;
    srand(time(NULL) ^ getpid() << 16);

    int groupSize = (rand() % 3 + 1);
    sem_closeDoor(SEM_DOOR, groupSize);

    semlock(SEM_MEMORY);
    bar->clients += groupSize;
    semunlock(SEM_MEMORY);
    printf(CLIENT_COL "[KLIENT %d] %d osob wchodzi do baru\n" RESET, getpid(), groupSize);    
    int ifOrder = rand() % 101;
    if(ifOrder <= 5){
        semlock(SEM_MEMORY);
        bar->clients -= groupSize;
        semunlock(SEM_MEMORY);

        printf(CLIENT_COL "[KLIENT %d] Rezygnujemy z zamowienia. Wychodzimy z baru!\n" RESET, getpid());

        sem_openDoor(SEM_DOOR, groupSize);
        semunlock(SEM_GENERATOR);
        detach_ipc();  
        return 0;
    }
    int pid = getpid();
    msg.mtype = MTYPE_CASHIER;
    msg.pid = pid;
    msgSend(msgCashier, &msg);
    msgReceive(msgClient, &msg, getpid(), 0);
    if(msg.payed != 1){
        printf(CLIENT_COL "[KLIENT %d] Platnosc sie nie powiodla, wychodze!!\n" RESET, getpid());
        semlock(SEM_MEMORY);
        bar->clients -= groupSize;
        semunlock(SEM_MEMORY);
        sem_openDoor(SEM_DOOR, groupSize);
        semunlock(SEM_GENERATOR);
        detach_ipc();
        return 0;
    }

    int foundTable = -1;
    while(foundTable == -1){
        //printf(CLIENT_COL "[KLIENT %d] Szukam stolika\n" RESET, getpid());
        semlock(SEM_MEMORY);
        for(int i = groupSize; i<=4; i++){
            for(int j = 0; j < bar->allTables; j++){
                Table *tab = &bar->tables[j];
                if(i == tab->capacity && tab->isReserved == 0 && tab->whoSits == 0){
                    foundTable = tab->id;
                    tab->freeSlots -= groupSize;
                    tab->whoSits = groupSize;
                    printf(CLIENT_COL "[KLIENT %d] Znalazlem stolik id %d (%d osobowy)\n" RESET, getpid(), foundTable, tab->capacity);
                    break;
                }
                if(groupSize == tab->whoSits && tab->freeSlots >= groupSize){
                    foundTable = tab->id;
                    printf(CLIENT_COL "[KLIENT %d] Dosiadam sie do stolika (%d osobowe) przy ktorym siedzi %d osob\n" RESET, getpid(), tab->capacity, tab->capacity - tab->freeSlots);
                    tab->freeSlots -= groupSize;
                    break;
                    }
                }
            if(foundTable != -1){
                break;
            }
        }
        semunlock(SEM_MEMORY);

        if(foundTable == -1){
            semlock(SEM_SEARCH);
        }
    }

    msg.mtype = MTYPE_WORKER;
    msg.pid = pid;
    msgSend(msgWorker, &msg);
    msgReceive(msgClient, &msg, getpid(), 0);

    printf(CLIENT_COL "[KLIENT %d] Jemy\n" RESET, getpid());
    //sleep(1);

    semlock(SEM_MEMORY);
    bar->tables[foundTable].freeSlots += groupSize;
    if(bar->tables[foundTable].freeSlots == bar->tables[foundTable].capacity){
        bar->tables[foundTable].whoSits = 0;
    }
    bar->clients -= groupSize;
    semunlock(SEM_MEMORY);

    sem_wakeWaiting(SEM_SEARCH);
    sem_openDoor(SEM_DOOR, groupSize);
    semunlock(SEM_GENERATOR);

    printf(CLIENT_COL "[KLIENT %d] Opuszczamy bar\n" RESET, getpid());
    detach_ipc();
    return 0;
}