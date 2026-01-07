#include "ipc.h"

int keep_running = 1;

int main(){
    BarState *bar = join_ipc();
    // msgbuf msg;
    printf(WORKER_COL "[PRACOWNIK] Rozpoczynam prace!\n" RESET);
    // while(keep_running){
    //     msgReceive(&msg, MTYPE_WORKER);
    //     printf(WORKER_COL "[PRACOWNIK] Wydaje danie dla grupy (PID: %d)\n" RESET, msg.pid);        
    // }
    // printf(WORKER_COL "[PRACOWNIK] Koncze prace!\n" RESET);

    while(1){
        printf(WORKER_COL "[PRACOWNIK] Stan baru: %d\n" RESET, bar->clients);
        sleep(2);
    }
    detach_ipc();
    return 0;
}