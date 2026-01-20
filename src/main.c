#include "include/ipc.h"

volatile sig_atomic_t running = 1; 
volatile sig_atomic_t fire = 0;

void handle_signals(int sig){
    if(sig == SIGINT || sig == SIGTERM){
        running = 0;
    }
    else if(sig == SIGQUIT){
        running = 0;
        fire = 1;
    }
}

int main(){
    struct sigaction sa;
    sa.sa_handler = handle_signals;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    int x1, x2, x3, x4;
    while (1) {
        printf("Podaj liczbe stolikow kolejno: 1-os, 2-os, 3-os, 4-os (oddziel spacja)\n");
        printf("PRZYKLAD: 1 2 3 4\n");
        
        if (scanf("%d %d %d %d", &x1, &x2, &x3, &x4) != 4) {
            printf("Błąd: Wprowadź cztery liczby całkowite!\n");
            while (getchar() != '\n'); // Czyszczenie bufora wejściowego
            continue;
        }

        if (x1 <= 0 || x2 <= 0 || x3 <= 0 || x4 <= 0) {
            printf("Błąd: Liczba wszystkich typów stolików musi być większa od 0!\n");
            continue;
        }
        break; // Dane są poprawne
    }

    int maxTables = x1 + x2 + (2*x3) + x4; 
    bar = init_ipc(x1, x2, x3, x4, maxTables);
    bar->mainPid = getpid();

    pid_t pid_generator = fork();
    if (pid_generator == 0){
        setpgid(getpid(), getpid());
        execl("bin/generator", "Generator", NULL);
        perror("exec generator error\n");
        exit(1);
    }
    if (pid_generator == -1){
        perror("fork generator error\n");
        exit(1);
    }

    pid_t pid_cashier = fork();
    if (pid_cashier == 0){
        execl("bin/cashier", "Kasjer", NULL);
        perror("exec cashier error\n");
        exit(1);
    }
    if (pid_cashier == -1){
        perror("fork cashier error\n");
        exit(1);
    }
    
    pid_t pid_worker = fork();
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
    if(fire == 1){
        //zabijamy cala grupe generatora 
        kill(-pid_generator, SIGQUIT);
        waitpid(pid_generator, NULL, 0);
        //gdy klienci wyszli z baru to wtedy pracownicy baru

        kill(pid_worker, SIGQUIT);
        kill(pid_cashier, SIGQUIT);
        waitpid(pid_worker, NULL, 0);
        waitpid(pid_cashier, NULL, 0);
    }
    else{
        kill(-pid_generator, SIGTERM);
        waitpid(pid_generator, NULL, 0);
        
        kill(pid_worker, SIGTERM);
        kill(pid_cashier, SIGTERM);
        waitpid(pid_worker, NULL, 0);
        waitpid(pid_cashier, NULL, 0);
    }
    detach_ipc();
    cleanup_ipc();
    return 0;
}