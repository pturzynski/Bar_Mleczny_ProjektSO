#include "include/ipc.h"

#define RED "\033[0;31m"

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
    msgbuf msg;

    semlock(SEM_MEMORY, 1);
    pid_t workerPid = bar->workerPid;
    bar->managerPid = getpid();
    int oldX3 = bar->x3;
    semunlock(SEM_MEMORY, 1);

    struct sigaction sa;
    sa.sa_handler = handle_signals;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
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
                        semlock(SEM_MEMORY, 1);
                        printf(RED "Podwojono stoliki 3 osobowe: bylo: %d teraz = %d allTables = %d\n" RESET, oldX3, bar->x3, bar->allTables);
                        semunlock(SEM_MEMORY, 1);
                    }
                    else{
                        printf(RED "Nie udalo sie podwoic liczby stolikow 3 osobowych\n" RESET);
                    }
                }
                else{
                    printf(RED "Juz raz podwoiles stoliki 3 osobowe\n" RESET);
                }
                break;
            case 2:
                int dontSend = 0;
                int x1, x2, x3, x4;
                x1 = x2 = x3 = x4 = 0;
                int resx1, resx2, resx3, resx4;
                resx1 = resx2 = resx3 = resx4 = 0;
                semlock(SEM_MEMORY, 1);
                for(int i = 0; i < bar->allTables; i++){
                    if(bar->tables[i].capacity == 1){
                        if(bar->tables[i].isReserved == 0){
                            x1++;
                        }
                        else{
                            resx1++;
                        }
                    }
                    else if(bar->tables[i].capacity == 2){
                        if(bar->tables[i].isReserved == 0){
                            x2++;
                        }
                        else{
                            resx2++;
                        }
                    }
                    else if(bar->tables[i].capacity == 3){
                        if(bar->tables[i].isReserved == 0){
                            x3++;
                        }
                        else{
                            resx3++;
                        }
                    }
                    else if(bar->tables[i].capacity == 4){
                        if(bar->tables[i].isReserved == 0){
                            x4++;
                        }
                        else{
                            resx4++;
                        }
                    }
                }
                semunlock(SEM_MEMORY, 1);

                printf("Stan stolikow (zarezerwowane / wolne)\n");
                printf("1os: %d / %d \n", resx1, x1);
                printf("2os: %d / %d \n", resx2, x2);
                printf("3os: %d / %d \n", resx3, x3);
                printf("4os: %d / %d \n", resx4, x4);
                printf("Podaj typ stolika do zarezerwowania (1-4)\n");
                int type, count;
                while(1){
                    if(scanf("%d", &type) != 1){
                        printf("Wprowadz liczbe\n");
                        while(getchar() != '\n');
                        continue;
                    }
                    if(type < 1 || type > 4){
                        printf("Liczba musi byc w przedziale 1-4\n");
                        continue;
                    }
                    break;
                }

                printf("Ile stolikow %d osobowych zarezerwowac?\n", type);
                while(1){
                    if(scanf("%d", &count) != 1){
                        printf("Wprowadz liczbe\n");
                        while(getchar() != '\n');
                        continue;
                    }
                    if(count < 0){
                        printf("Liczba musi być dodatnia\n");
                        continue;
                    }

                    int available = 0;
                    if(type == 1){
                        available = x1;
                    }
                    else if(type == 2){
                        available = x2;
                    }
                    else if(type == 3){
                        available = x3;
                    }
                    else if(type == 4){
                        available = x4;
                    }

                    if(count > available){
                        dontSend = 1;
                        printf(RED "Nie ma tylu stolikow\n" RESET);
                        break;
                    }
                    break;
                }
                if(dontSend == 1){
                    dontSend = 0;
                    break;
                }
                msg.tableType = type;
                msg.count = count;
                msg.mtype = MTYPE_RESERVATION;

                msgSend(msgStaff, &msg);
                kill(bar->workerPid, SIGUSR2);
                pause();
                if(success == 1){
                    printf(RED "Zarezerwowano %d stolikow %d osobowych\n" RESET, count, type);
                }
                else{
                    printf(RED "Nie udalo sie zarezerwowac stolikow\n" RESET);
                }
                break;
            case 3:
                printf(RED "POZAR!!!\n" RESET);
                kill(-bar->mainPid, SIGTERM);
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