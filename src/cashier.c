#include "ipc.h"

int keep_running = 1;

void handle_signal(int sig) {
    keep_running = 0;
}

int main(){
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    BarState *bar = join_ipc();
    msgbuf msg;
    printf(CASHIER_COL "[KASJER] Kasa została otwarta\n" RESET);

    while(keep_running){
        int foundTable = -1;

        msgReceive(&msg, MTYPE_CASHIER);
        printf(CASHIER_COL "[KASJER] Szukam stolika %d osobowego dla klienta (%d)\n" RESET, msg.groupSize, msg.pid);
        semlock(SEM_MEMORY);
        //i to rozmiar grupy, zaczynamy szukac od stolikow rownym rozmiarowi grupy 
        for(int i = msg.groupSize; i<=4; i++){
            for(int j = 0; j < bar->allTables; j++){
                Table *tab = &bar->tables[j];
                if(i == tab->capacity && tab->isReserved == 0 && tab->whoSits == 0){
                    foundTable = tab->id;
                    tab->freeSlots -= msg.groupSize;
                    tab->whoSits = msg.groupSize;
                    printf(CASHIER_COL "[KASJER] Znalazlem stolik (nr %d, %d osobowy) dla klienta (%d)\n" RESET, foundTable, tab->capacity, msg.pid);
                    break;
                }
                if(msg.groupSize == tab->whoSits && tab->freeSlots >= msg.groupSize){
                    foundTable = tab->id;
                    printf(CASHIER_COL "[KASJER] Znalazlem stolik (%d , %d osobowy) dla klienta (%d). Siedzi przy aktualnie %d osob\n" RESET, foundTable, tab->capacity, 
                        msg.pid, tab->capacity - tab->freeSlots);
                    tab->freeSlots -= msg.groupSize;
                    break;
                }
                if(foundTable != -1){
                    break;
                }
            }
        }
        if(foundTable != -1){
            msg.tableId = foundTable;
            msg.order = 1;
        }
        else{ 
            msg.order = 0;
        }
        msg.mtype = msg.pid;
        semunlock(SEM_MEMORY);
        msgSend(&msg);
    }
    printf(CASHIER_COL "[KASJER] Kasa zamknieta\n" RESET);
    detach_ipc();
    return 0;
}