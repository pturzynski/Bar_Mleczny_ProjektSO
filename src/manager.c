#include "include/ipc.h"

volatile sig_atomic_t success = 0;

void print_menu(){
    printf("----MENU----\n");
    printf("1. PODWOJENIE ILOSCI STOLIKOW X3 (mozliwe tylko raz)\n");
    printf("2. Rezerwacja stolikow\n");
    printf("3. POZAR / opuszczanie baru\n");
    printf("4. Wyjscie\n");
}

void handle_signals(int sig){
    if(sig == SIGUSR1){ //sukces
        success = 1;
    }
    if(sig == SIGUSR2){ //niepowodzenie
        success = 0;
    }
}

int main(){
    BarState *bar = join_ipc();
    semlock(SEM_MEMORY);
    pid_t workerPid = bar->workerPid;
    bar->managerPid = getpid();
    int oldX3 = bar->x3;
    semunlock(SEM_MEMORY);

    struct sigaction sa;
    sa.sa_handler = handle_signals;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    int doubleX3 = 0;
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
                if(doubleX3 == 0){
                    kill(workerPid, SIGUSR1);
                    pause();
                    if(success == 1){
                        doubleX3 = 1;
                        semlock(SEM_MEMORY);
                        printf("Podwojono stoliki 3 osobowe: bylo: %d teraz = %d allTables = %d\n", oldX3, bar->x3, bar->allTables);
                        semunlock(SEM_MEMORY);
                    }
                    else{
                        printf("Nie udalo sie podwoic liczby stolikow 3 osobowych\n");
                    }
                }
                else{
                    printf("Juz raz podwoiles stoliki 3 osobowe\n");
                }
                break;
            case 2:
                break;
            case 3:
                printf("POZAR!!!\n");
                kill(bar->mainPid, SIGQUIT);
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