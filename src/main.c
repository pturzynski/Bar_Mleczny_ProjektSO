#include "ipc.h"
#include <signal.h>

int keep_running = 1;

void handle_sigint(int sig) {
    printf("\n[Serwer] Otrzymano sygnał %d (CTRL+C). Zamykanie baru...\n", sig);
    keep_running = 0;
}

int main(){
    signal(SIGINT, handle_sigint);
    int x1, x2, x3, x4;
    printf("Podaj liczbe stolikow kolejno: 1-os, 2-os, 3-os, 4-os (oddziel spacja)\n");
    printf("PRZYKLAD: 1 2 3 4\n");
    scanf("%d %d %d %d", &x1, &x2, &x3, &x4);

    BarState *bar = init_ipc(x1, x2, x3, x4);
    printf("Ilosc stolikow %d , %d , %d , %d\n", bar->x1, bar->x2, bar->x3, bar->x4);
    
    int pid_generator = fork();
    if (pid_generator == 0){
        execl("./clientgenerator", "generator", NULL);
        perror("exec generator error");
        exit(1);
    }
    if (pid_generator == -1){
        perror("fork error");
        exit(1);
    }
    
    while(keep_running){
        printf("[MAIN] Aktualnie w barze %d klientów\n", bar->clients);
        sleep(5);
    }

    detach_ipc();
    cleanup_ipc();
    return 0;
}