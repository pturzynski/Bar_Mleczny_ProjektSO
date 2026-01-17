#include "include/ipc.h"
void print_menu(){
    printf("----MENU----\n");
    printf("1. PODWOJENIE ILOSCI STOLIKOW X3 (mozliwe tylko raz)\n");
    printf("2. Rezerwacja stolikow\n");
    printf("3. POZAR / opuszczanie baru\n");
    printf("4. Wyjscie\n");
}

int main(){
    BarState *bar = join_ipc();
    int option;
    while(1){
        print_menu();
        printf("Wybierz opcje z MENU\n");
        if(scanf("%d", &option) != 1){
            printf("Wprowadz liczbe\n");
            while(getchar() != '\n');
            continue;
        }
        switch(option){
            case 1:
                semlock(SEM_MEMORY);
                if(bar->flagDoubleX3 != 0){
                    printf("Juz raz podwoiles stoliki\n");
                    break;
                }
                else{
                    int x3 = bar->x3;
                    int newTables = bar->allTables + x3;
                    if(newTables > bar->maxTables){
                        printf("Nie mozna podwoic stolikow\n");
                        semunlock(SEM_MEMORY);
                        break;
                    }
                    int ind = bar->allTables;
                    for(int i = 0; i < x3; i++){
                        bar->tables[ind].id = ind;
                        bar->tables[ind].capacity = 3;
                        bar->tables[ind].whoSits = 0;
                        bar->tables[ind].freeSlots = 3;
                        bar->tables[ind].isReserved = 0;
                        ind++;
                    }
                    bar->x3 += x3;
                    bar->allTables = newTables;
                    bar->maxClients += 3 * x3;
                    bar->flagDoubleX3 = 1;
                    semunlock(SEM_MEMORY);
                    printf("Podwoilem liczbe stolikow 3-osobowych. Teraz x3 = %d, allTables = %d\n", bar->x3, bar->allTables);
                }
                break;
            case 2:
                semlock(SEM_MEMORY);
                printf("%d\n", bar->maxClients);
                printf("%d\n", bar->clients);
                break;
            case 3:
                printf("3\n");
                break;
            case 4:
                detach_ipc();
                return 0;
            default:
                printf("Musisz wybrac jakas opcje z MENU");
                break;
        }
    }
    detach_ipc();
    return 0;
}