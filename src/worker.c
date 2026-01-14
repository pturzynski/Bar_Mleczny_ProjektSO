#include "ipc.h"

int keep_running = 1;

int main(){
    BarState *bar = join_ipc();
    msgbuf msg;
    printf(WORKER_COL "[PRACOWNIK] Rozpoczynam prace!\n" RESET);

    while(keep_running){
        msgReceive(&msg, MTYPE_WORKER);
        
        if(msg.action == WORKER_FOOD){
            printf(WORKER_COL "[PRACOWNIK] Nakladam danie dla klienta %d\n" RESET, msg.pid);
            msg.mtype = msg.pid;
            msgSend(&msg);
        }
        if(msg.action == WORKER_CLEAN){
            printf(WORKER_COL "[PRACOWNIK] Sprzatam stolik\n" RESET);
            semlock(SEM_MEMORY);
            bar->tables[msg.tableId].freeSlots += msg.groupSize;
            if(bar->tables[msg.tableId].freeSlots == bar->tables[msg.tableId].capacity){
                bar->tables[msg.tableId].whoSits = 0;
            }
            semunlock(SEM_MEMORY);
            semunlock(SEM_MSG_LIMIT);
        }
    }
    printf(WORKER_COL "[PRACOWNIK] Koncze prace!\n" RESET);
    detach_ipc();
    return 0;
}
