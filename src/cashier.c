#include "ipc.h"

int main(){
    BarState *bar = join_ipc();
    printf("[KASJER] Kasa została otwarta\n");

    while(true){
        semlock(SEM_MEMORY);
        printf("[KASJER] Aktualnie w barze %d osob\n", bar->clients);
        semunlock(SEM_MEMORY);
        sleep(5);
    }
    
    detach_ipc();
    return 0;
}