#include "ipc.h"

int keep_running = 1;

int main(){
    BarState *bar = join_ipc();
    msgbuf msg;
    printf("[KASJER] Kasa została otwarta\n");

    while(keep_running){
        msgReceive(&msg, 1);
        printf("[KASJER] Witamy grupe klientow ID: %d, ktora liczy %d osob w barze\n", msg.pid, msg.groupSize);
        printf("[KASJER] Aktualnie  barze jest %d\n", bar->clients);
        sleep(1);
    }
    printf("[KASJER] Kasa zamknieta\n");
    detach_ipc();
    return 0;
}