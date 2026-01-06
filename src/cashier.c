#include "ipc.h"

int keep_running = 1;

int main(){
    BarState *bar = join_ipc();
    msgbuf msg;
    printf(CASHIER_COL "[KASJER] Kasa została otwarta\n" RESET);

    while(keep_running){
        int foundTable = -1;

        msgReceive(&msg, 1);
        printf(CASHIER_COL "[KASJER] Zamowienie od grupy (PID): %d, ktora liczy %d osob zotalo przyjete\n" RESET, msg.pid, msg.groupSize);
        for(int i = 0; i < bar->allTables; i++){
            if (bar->tables[i].capacity == msg.groupSize && 
                bar->tables[i].freeSlots >= msg.groupSize &&
                bar->tables[i].isReserved == 0){
                    foundTable = bar->tables[i].id;
                    semlock(SEM_MEMORY);
                    bar->tables[i].freeSlots -= msg.groupSize;
                    bar->tables[i].whoSits = msg.groupSize; 
                    printf(CASHIER_COL "[KASJER] Znalazlem stolik o id: %d dla PID %d\n" RESET, foundTable, msg.pid);
                    semunlock(SEM_MEMORY);
                    break;
                }
        }
        msg.mtype = msg.pid;
        msg.tableId = foundTable;
        if(foundTable == -1){
            msg.order = 0;
            msgSend(&msg);
        }
        else{
            msg.order = 1;
        }
        msgSend(&msg);

    }
    printf(CASHIER_COL "[KASJER] Kasa zamknieta\n" RESET);
    detach_ipc();
    return 0;
}