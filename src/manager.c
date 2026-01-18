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
    msgbuf msg;
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
                if(bar->flagDoubleX3 == 0){
                    printf("Liczba stolikow x3 aktualnie: %d\n", bar->x3);
                    kill(bar->workerPid, SIGUSR1);
                    msgReceive(msgStaff, &msg, MTYPE_STAFF, 0);
                    if(msg.success == 1){
                        printf("Podwojono liczbe stolikow 3-osobowych. Teraz x3 = %d, allTables = %d\n", bar->x3, bar->allTables);
                    }
                    else{
                        printf("Nie udalo sie podwoic stolikow\n");
                    }
                }
                else{
                    printf("Juz raz podwojono stoliki x3\n");
                }
                break;
            case 2:
                semlock(SEM_MEMORY);
                printf("%d\n", bar->maxClients);
                printf("%d\n", bar->clients);
                semunlock(SEM_MEMORY);
                break;
            case 3:
                semlock(SEM_MEMORY);
                bar->flagFire = 1;
                semunlock(SEM_MEMORY);
                sem_wakeWaiting();
                kill(bar->mainPid, SIGUSR2);
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