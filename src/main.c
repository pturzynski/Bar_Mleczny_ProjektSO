#include "include/ipc.h"

volatile sig_atomic_t running = 1; 

void handle_signals(int sig){
    running = 0;
}

int main(){
    struct sigaction sa;
    sa.sa_handler = handle_signals;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int x1, x2, x3, x4;
    printf("Podaj liczbe stolikow kolejno: 1-os, 2-os, 3-os, 4-os (oddziel spacja)\n");
    printf("PRZYKLAD: 1 2 3 4\n");
    scanf("%d %d %d %d", &x1, &x2, &x3, &x4);
    if (x1 <= 0 || x2 <= 0 || x3 <= 0 || x4 <= 0){
        fprintf(stderr, "Błąd: liczba stolików musi być większa od 0!\n");
        exit(1);
    }
    int maxTables = x1 + x2 + (2*x3) + x4; 
    bar = init_ipc(x1, x2, x3, x4, maxTables);

    int pid_generator = fork();
    if (pid_generator == 0){
        execl("bin/generator", "Generator", NULL);
        perror("exec generator error\n");
        exit(1);
    }
    if (pid_generator == -1){
        perror("fork generator error\n");
        exit(1);
    }

    int pid_cashier = fork();
    if (pid_cashier == 0){
        execl("bin/cashier", "Kasjer", NULL);
        perror("exec cashier error\n");
        exit(1);
    }
    if (pid_cashier == -1){
        perror("fork cashier error\n");
        exit(1);
    }
    
    int pid_worker = fork();
    if (pid_worker == 0){
        execl("bin/worker", "Pracownik", NULL);
        perror("exec cashier error\n");
        exit(1);
    }
    if (pid_worker == -1){
        perror("fork worker error\n");
        exit(1);
    }

    while(running) {
        pause();
    }

    detach_ipc();
    cleanup_ipc();
    return 0;
}