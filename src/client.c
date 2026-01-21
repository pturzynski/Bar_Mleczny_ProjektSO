#include "include/ipc.h"

void handle_signal(int sig){
    if (sig == SIGQUIT){
        logger(CLIENT_COL "[KLIENT %d] POZAR! UCIEKAMY" RESET, getpid());
        detach_ipc();
        exit(0);
    }
}

int main(){
    srand(time(NULL) ^ getpid() << 16);

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGQUIT, &sa, NULL);
    
    BarState *bar = join_ipc();
    msgbuf msg;

    int groupSize = (rand() % 3 + 1);
    sem_closeDoor(SEM_DOOR, groupSize);

    int ifOrder = rand() % 101;
    if(ifOrder <= 5){
        printf(CLIENT_COL "[KLIENT %d] Nie zamawiamy nic!\n" RESET, getpid());
        sem_openDoor(SEM_DOOR, groupSize);
        semunlock(SEM_GENERATOR);
        detach_ipc();  
        return 0;
    }

    semlock(SEM_MEMORY);
    bar->clients += groupSize;
    semunlock(SEM_MEMORY);
    printf(CLIENT_COL "[KLIENT %d] Idzimy zamowic posilek (%d osob)\n" RESET, getpid(), groupSize);    

    int pid = getpid();
    msg.mtype = MTYPE_CASHIER;
    msg.pid = pid;
    printf(CLIENT_COL "[KLIENT %d] Zamawiamy jedzenie\n" RESET, getpid());
    msgSend(msgCashier, &msg);
    msgReceive(msgClient, &msg, getpid());
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

    //szukanie stolika
    int foundTable = -1;
    int cap = -1; //poj stolika
    int whos = 0; //ile osopb siedzi
    while(foundTable == -1){
        semlock(SEM_MEMORY);
        for(int i = groupSize; i<=4; i++){
            for(int j = 0; j < bar->allTables; j++){
                Table *tab = &bar->tables[j];
                if(i == tab->capacity && tab->isReserved == 0 && tab->whoSits == 0){
                    foundTable = tab->id;
                    tab->freeSlots -= groupSize;
                    tab->whoSits = groupSize;
                    cap = tab->capacity;
                    break;
                }
                if(groupSize == tab->whoSits && tab->freeSlots >= groupSize){
                    foundTable = tab->id;
                    whos = tab->capacity - tab->freeSlots; //ile osob siedzi
                    cap = tab->capacity;
                    tab->freeSlots -= groupSize;
                    break;
                    }
                }
            if(foundTable != -1){
                break;
            }
        }
        semunlock(SEM_MEMORY);

        //jesli nie znajde to blokuje sie na semaforze
        if(foundTable == -1){
            semlock(SEM_SEARCH);
        }
    }//while

    msg.mtype = MTYPE_WORKER;
    msg.pid = pid;
    printf(CLIENT_COL "[KLIENT %d] Odbieramy zamowienie!\n" RESET, getpid());
    msgSend(msgWorker, &msg);
    msgReceive(msgClient, &msg, getpid());
    printf(CLIENT_COL "[KLIENT %d] Siadamy przy stoliku id %d (%d osobowy, siedzi przy nim %d osob)\n" RESET, getpid(), foundTable, cap, whos);
    printf(CLIENT_COL "[KLIENT %d] Jemy\n" RESET, getpid());
    sleep(2);
    //zwalnianie stolika
    semlock(SEM_MEMORY);
    bar->tables[foundTable].freeSlots += groupSize;
    if(bar->tables[foundTable].freeSlots == bar->tables[foundTable].capacity){
        bar->tables[foundTable].whoSits = 0;
    }
    bar->clients -= groupSize;
    semunlock(SEM_MEMORY);

    printf(CLIENT_COL "[KLIENT %d] Odnosimy naczynia i opuszczamy bar (%d osob)\n" RESET, getpid(), groupSize);
    
    sem_wakeAll(SEM_SEARCH);
    sem_openDoor(SEM_DOOR, groupSize);
    semunlock(SEM_GENERATOR);
    detach_ipc();
    return 0;
}