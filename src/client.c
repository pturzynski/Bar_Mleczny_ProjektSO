#include "include/ipc.h"
#include <pthread.h>

pthread_mutex_t priceMutex = PTHREAD_MUTEX_INITIALIZER;
int totalPrice = 0;

void* clientRoutine(void* arg){
    intptr_t id = (intptr_t)arg;

    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)pthread_self();
    int choice = (rand_r(&seed) % 3) + 1;
    int price = 0;

    if(choice == 1){
        logger(CLIENT_COL "[KLIENT %d | Osoba %ld] Zamawiam zupe (8zl)!" RESET, getpid(), id);
        price = 8;
    }
    else if(choice == 2){
        logger(CLIENT_COL "[KLIENT %d | Osoba %ld] Zamawiam danie glowne (28zl)!" RESET, getpid(), id);
        price = 26;
    }
    else if(choice == 3){
        logger(CLIENT_COL "[KLIENT %d | Osoba %ld] Zamawiam zestaw, zupa + danie glowne (30zl)!" RESET, getpid(), id);
        price = 30;
    }

    pthread_mutex_lock(&priceMutex);
    totalPrice += price;
    pthread_mutex_unlock(&priceMutex);

    return NULL;
}

void* clientEatTime(void* arg){
    intptr_t id = (intptr_t)arg;
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)pthread_self();
    int eatTime = (rand_r(&seed) % 5) + 2;
    logger(CLIENT_COL "[KLIENT %d | Osoba %ld] Jem (%d sek)!" RESET, getpid(), id, eatTime);
    //sleep(eatTime);

    return NULL;
}

void handle_signal(int sig) {
    if(sig == SIGINT){
        loggerClose();
        _exit(0);
    }
    if(sig == SIGTERM){
        //logger(CLIENT_COL "[KLIENT %d] POZAR!!!" RESET, getpid());
        loggerClose();
        _exit(0);
    }
}

int main(){
    loggerOpen();
    totalPrice = 0;
    
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    bar = join_ipc();
    msgbuf msg;
    srand(time(NULL) ^ getpid() << 16);
    int groupSize = (rand() % 3) + 1;
    int ifOrder = rand() % 101;
    
    sem_closeDoor(SEM_DOOR, groupSize, 1);
    logger(CLIENT_COL "[KLIENT %d | %d osob] Wchodzimy do baru!" RESET, getpid(), groupSize);
    //nie zamawiamy i wychodzimy
    if(ifOrder <= 5){
        logger(CLIENT_COL "[KLIENT %d | %d osob] Nie zamawiamy nic. Wychodzimy!" RESET, getpid(), groupSize);
        sem_openDoor(SEM_DOOR, groupSize, 1);
        loggerClose();
        detach_ipc();
        return 0;
    }

    //jesli caly bar jest zarezerwowany to tez wychodzimy
    int unreservedTables = 0;
    semlock(SEM_MEMORY, 1);
    for(int i = 0; i < bar->allTables; i++){
        if(bar->tables[i].isReserved == 0){
            unreservedTables++;
        }
    }
    semunlock(SEM_MEMORY, 1);
    if(unreservedTables == 0){
        logger(CLIENT_COL "[KLIENT %d | %d osob] Wszystkie stoliki sa zarezerwowane. Wychodzimy!" RESET, getpid(), groupSize);
        sem_openDoor(SEM_DOOR, groupSize, 1);
        loggerClose();
        detach_ipc();
        return 0;
    }

    //teraz mozemy byc klientami w barze, ktorzy moga zamowic wiec wiekszamy bar clients
    semlock(SEM_MEMORY, 1);
    bar->clients += groupSize;
    semunlock(SEM_MEMORY, 1);

    logger(CLIENT_COL "[KLIENT %d | %d osob] Szukamy stolika %d osobowego" RESET, getpid(), groupSize, groupSize);
    //szukanie stolika

    int foundTable = -1;
    while(foundTable == -1){
        semlock(SEM_MEMORY, 1);

        unreservedTables = 0;
        for(int k = 0; k < bar->allTables; k++){
            if(bar->tables[k].isReserved == 0){
                unreservedTables++;
            }
        }
        if(unreservedTables == 0){
            logger(CLIENT_COL "[KLIENT %d | %d osob] Wszystkie stoliki sa zarezerwowane. Wychodzimy" RESET, getpid(), groupSize);
            bar->clients -= groupSize;
            semunlock(SEM_MEMORY, 1);
            sem_wakeOne(SEM_SEARCH);
            sem_openDoor(SEM_DOOR, groupSize, 1);
            loggerClose();
            detach_ipc();
            exit(0);
        }

        for(int i = groupSize; i<=4; i++){
            for(int j = 0; j < bar->allTables; j++){
                Table *tab = &bar->tables[j];
                if(groupSize == tab->whoSits && tab->freeSlots >= groupSize){
                    foundTable = tab->id;
                    tab->freeSlots -= groupSize;
                    break;
                }
                if(i == tab->capacity && tab->isReserved == 0 && tab->whoSits == 0){
                    foundTable = tab->id;
                    tab->freeSlots -= groupSize;
                    tab->whoSits = groupSize;
                    break;
                }
            }
            if(foundTable != -1){
                break;
            }
        }
        semunlock(SEM_MEMORY, 1);

        //jesli nie znajde to blokuje sie na semaforze
        if(foundTable == -1){
            semlock(SEM_SEARCH, 0);
        }
    }//while

    pthread_t members[2];
    if(groupSize > 1){
        for(int i = 0; i < groupSize - 1; i++){
            pthread_create(&members[i], NULL, clientRoutine, (void*)(intptr_t)(i + 2));
        }
    }
    clientRoutine((void*)(intptr_t)1);

    if(groupSize > 1){
        for(int i = 0; i < groupSize - 1; i++){
            pthread_join(members[i], NULL);
        }
    }

    msg.mtype = MTYPE_CASHIER;
    msg.pid = getpid();
    msg.price = totalPrice;
    semlock(SEM_ORDER, 1);
    msgSend(msgOrder, &msg);
    msgReceive(msgOrder, &msg, getpid());
    semunlock(SEM_ORDER, 1);
    logger(CLIENT_COL "[KLIENT %d | %d osob] Zaplacilismy za jedzenie %d zl" RESET, getpid(), groupSize, totalPrice);

    msg.mtype = MTYPE_WORKER;
    msg.pid = getpid();
    semlock(SEM_FOOD, 1);
    msgSend(msgFood, &msg);
    msgReceive(msgFood, &msg, getpid());
    semunlock(SEM_FOOD, 1);
    logger(CLIENT_COL "[KLIENT %d | %d osob] Odebralismy jedzenie od pracownika" RESET, getpid(), groupSize);

    if(groupSize > 1){
        for(int i = 0; i < groupSize - 1; i++){
            pthread_create(&members[i], NULL, clientEatTime, (void*)(intptr_t)(i + 2));
        }
    }
    clientEatTime((void*)(intptr_t)1);

    if(groupSize > 1){
        for(int i = 0; i < groupSize - 1; i++){
            pthread_join(members[i], NULL);
        }
    }

    semlock(SEM_MEMORY, 1);
    bar->tables[foundTable].freeSlots += groupSize;
    if(bar->tables[foundTable].freeSlots == bar->tables[foundTable].capacity){
        bar->tables[foundTable].whoSits = 0;
    }
    bar->clients -= groupSize;
    semunlock(SEM_MEMORY, 1);

    logger(CLIENT_COL "[KLIENT %d | %d osob] Odnosimy naczynia i opuszczamy bar" RESET, getpid(), groupSize);
    sem_wakeOne(SEM_SEARCH);
    sem_openDoor(SEM_DOOR, groupSize, 1);
    loggerClose();
    detach_ipc();
    return 0;
}