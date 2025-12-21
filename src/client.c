#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
 
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
    semlock();
    bar->clients += groupSize;
    semunlock();
    printf("[KLIENT %d] Grupa %d osobowa wchodzi do baru\n", getpid(), groupSize);
    sleep(15);
    semlock();    
    bar->clients -= groupSize;
    semunlock();
    printf("[KLIENT %d] Wychodzimy!\n", getpid());
    detach_ipc();  
    return 0;
}