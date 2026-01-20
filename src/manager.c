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
                break;
            case 2:
                break;
            case 3:
                printf("POZAR\n");
                kill(bar->mainPid, SIGQUIT);
                detach_ipc();
                return 0;
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