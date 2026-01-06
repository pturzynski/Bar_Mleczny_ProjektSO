#include "ipc.h"
#include <signal.h>

int keep_running = 1;

void handle_sigint(int sig) {
    printf("\n[MAIN] Otrzymano sygnal ctrl+c zamykanie baru\n");
    keep_running = 0;
}

int main(){
    signal(SIGINT, handle_sigint);
    int x1, x2, x3, x4;
    int maxTables = x1 + x2 + (2*x3) + x4; 
    printf("Podaj liczbe stolikow kolejno: 1-os, 2-os, 3-os, 4-os (oddziel spacja)\n");
    printf("PRZYKLAD: 1 2 3 4\n");
    scanf("%d %d %d %d", &x1, &x2, &x3, &x4);
    if (x1 <= 0 || x2 <= 0 || x3 <= 0 || x4 <= 0){
        fprintf(stderr, "Błąd: liczba stolików musi być większa od 0!\n");
        exit(1);
    }

    BarState *bar = init_ipc(x1, x2, x3, x4, maxTables);

    int pid_generator = fork();
    if (pid_generator == 0){
        execl("./generator", "Generator", NULL);
        perror("exec generator error\n");
        exit(1);
    }
    if (pid_generator == -1){
        perror("fork generator error\n");
        exit(1);
    }

    int pid_cashier = fork();
    if (pid_cashier == 0){
        execl("./cashier", "Kasjer", NULL);
        perror("exec cashier error\n");
        exit(1);
    }
    if (pid_cashier == -1){
        perror("fork cashier error\n");
        exit(1);
    }
    
    while(keep_running) {
        pause();
    }
    
    detach_ipc();
    cleanup_ipc();
    return 0;
}