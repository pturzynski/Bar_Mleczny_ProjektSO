#include "ipc.h"

 
int getGroupSize(){
    srand(time(NULL) ^ getpid() << 16);
    if ((rand() % 101) <= 5){
        return 0;
    };
    int groupSize = (rand() % 3 + 1);
    return groupSize;
}

int main(){
    int groupSize = getGroupSize();
    if (groupSize == 0){
        printf("Rezygnuje!\n");
        return 0;
    }
    BarState *bar = join_ipc();
    printf("[KLIENT %d] Grupa %d osobowa staje w kolejce do kasy\n", getpid(), groupSize);
    semlock(SEM_CASHIER);
    semlock(SEM_MEMORY);
    bar->clients += groupSize;
    semunlock(SEM_MEMORY);
    printf("[KLIENT %d] Zamawiam\n", getpid());
    sleep(2);
    semunlock(SEM_CASHIER);
    sleep(10);
    printf("[KLIENT %d] Zjedlismy, Wychdzimy! (Grupa %d osob wychodzi z baru\n", getpid(), groupSize);\
    semlock(SEM_MEMORY); 
    bar->clients -= groupSize;
    semunlock(SEM_MEMORY);
    detach_ipc();  
    return 0;
}