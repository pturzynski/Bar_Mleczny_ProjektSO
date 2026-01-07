#include "ipc.h"

int keep_running = 1;

int main(){
    BarState *bar = join_ipc();
    msgbuf msg;
    printf(CASHIER_COL "[KASJER] Kasa została otwarta\n" RESET);

    while(keep_running){
        int foundTable = -1;

        msgReceive(&msg, MTYPE_CASHIER);
        printf(CASHIER_COL "[KASJER] Szukam stolika %d osobowego dla klienta (%d)\n" RESET, msg.groupSize, msg.pid);
        for(int i = 0; i < bar->allTables; i++){
            if(bar->tables[i].capacity == msg.groupSize && 
               bar->tables[i].freeSlots >= msg.groupSize && 
               bar->tables[i].isReserved == 0){
                    printf("1 if\n");
                    foundTable = bar->tables[i].id;
                    semlock(SEM_MEMORY);
                    bar->tables[i].freeSlots -= msg.groupSize;
                    bar->tables[i].whoSits = msg.groupSize;
                    printf(CASHIER_COL "[KASJER] Znalazlem stolik (nr %d, %d osobowy) dla klienta (%d)\n" RESET, foundTable, bar->tables[i].capacity, msg.pid);
                    semunlock(SEM_MEMORY);
                    printf(CASHIER_COL "[KASJER] Zamowienie przyjete\n");
                    break;
                }

            if(bar->tables[i].capacity >= msg.groupSize &&
               bar->tables[i].freeSlots >= msg.groupSize && 
               bar->tables[i].isReserved == 0){
                    if(bar->tables[i].whoSits == 0){
                        printf("2 if\n");
                        foundTable = bar->tables[i].id;
                        semlock(SEM_MEMORY);
                        bar->tables[i].whoSits = msg.groupSize;
                        bar->tables[i].freeSlots -= msg.groupSize;
                        printf(CASHIER_COL "[KASJER] Znalazlem stolik (%d , %d osobowy) dla PID %d\n" RESET, foundTable, bar->tables[i].capacity, msg.pid);
                        semunlock(SEM_MEMORY);
                        printf(CASHIER_COL "[KASJER] Zamowienie przyjete\n");
                        break;
                    }

                    if(bar->tables[i].whoSits == msg.groupSize){
                        printf("3 if\n");
                        foundTable = bar->tables[i].id;
                        semlock(SEM_MEMORY);
                        bar->tables[i].freeSlots -= msg.groupSize;
                        printf(CASHIER_COL "[KASJER] Znalazlem stolik (%d , %d osobowy) dla PID %d. Siedzi przy aktualnie %d osob\n" RESET, foundTable, bar->tables[i].capacity, msg.pid,
                        bar->tables[i].capacity - bar->tables[i].freeSlots);
                        semunlock(SEM_MEMORY);
                        printf(CASHIER_COL "[KASJER] Zamowienie przyjete\n");
                        break;
                    }
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
            msgSend(&msg);
        }
    }
    printf(CASHIER_COL "[KASJER] Kasa zamknieta\n" RESET);
    detach_ipc();
    return 0;
}