#include "include/ipc.h"
#include <pthread.h>

volatile sig_atomic_t running = 1; 
volatile sig_atomic_t fire = 0;
volatile int counter = 0;

pthread_t id_generator = -1;
pthread_t id_reaper = -1;
pid_t pid_cashier = -1;
pid_t pid_worker = -1;
pid_t personel = -1;

void handle_signal(int sig){
    if(sig == SIGINT){
        running = 0;
    }
    if(sig == SIGTERM){
        running = 0;
        fire = 1;
    }
    if(sig == SIGTSTP){
        if(personel > 0) {
            kill(-personel, SIGTSTP);
        }
        raise(SIGSTOP);
    }
    if(sig == SIGCONT){
        if(personel > 0) {
            kill(-personel, SIGCONT);
        }
    }
}

void* generatorRoutine(){
    while(running){
        int res = semlock(SEM_GENERATOR, 0);
        if(res == -1){
            if(!running){
                break;
            }
            continue;
        }
        if(res == -2){
            break;
        }
        if(!running){
            semunlock(SEM_GENERATOR, 0);
            break;
        }
        pid_t pid = fork();
        if(pid == 0){
            execl("bin/client", "Klient", NULL);
            perror("execl client failed");
            semunlock(SEM_GENERATOR, 0);
            _exit(1);
        }
        else if(pid == -1){
            semunlock(SEM_GENERATOR, 0);
            perror("generator fork error");
        }
        else{
            counter++;
        }
    }
    return NULL;
}

void* reaperRoutine(){
    while(running){
        pid_t deadChild;
        while((deadChild = waitpid(-1, NULL, WNOHANG)) > 0){
            if(deadChild != pid_cashier && deadChild != pid_worker){
                semunlock(SEM_GENERATOR, 0);
            }
        }
    }
    return NULL;
}

int main(){
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
    sigaction(SIGCONT, &sa, NULL);

    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd != -1){
        close(fd);
    }

    int x1, x2, x3, x4;
    while (1) {
        printf("Podaj liczbe stolikow kolejno: 1-os, 2-os, 3-os, 4-os (oddziel spacja)\n");
        printf("PRZYKLAD: 1 2 3 4\n");
        
        if (scanf("%d %d %d %d", &x1, &x2, &x3, &x4) != 4) {
            printf("Blad: Wprowadz cztery liczby\n");
            while (getchar() != '\n');
            continue;
        }
        if (x1 <= 0 || x2 <= 0 || x3 <= 0 || x4 <= 0) {
            printf("Blad: Liczby musza byc dodatnie\n");
            while (getchar() != '\n');
            continue;
        }
        break;
    }

    int maxTables = x1 + x2 + (2*x3) + x4; 
    bar = init_ipc(x1, x2, x3, x4, maxTables);
    bar->mainPid = getpid();

    loggerOpen();
    logger("[MAIN] Bar uruchomiony");

    personel = 0;
    pid_cashier = fork();
    if (pid_cashier == 0){
        setpgid(0, 0);
        execl("bin/cashier", "Kasjer", NULL);
        perror("exec cashier error\n");
        detach_ipc();
        _exit(1);
    }
    if (pid_cashier == -1){
        perror("fork cashier error\n");
        detach_ipc();
        cleanup_ipc();
        exit(1);
    }
    
    personel = pid_cashier;

    pid_worker = fork();
    if (pid_worker == 0){
        setpgid(0, personel);
        execl("bin/worker", "Pracownik", NULL);
        perror("exec worker error\n");
        detach_ipc();
        _exit(1);
    }
    if (pid_worker == -1){
        perror("fork worker error\n");
        detach_ipc();
        kill(pid_cashier, SIGTERM);
        waitpid(pid_cashier, NULL, 0);
        cleanup_ipc();
        exit(1);
    }

    int res = pthread_create(&id_reaper, NULL, reaperRoutine, NULL);
    if(res != 0){
        errno = res;
        perror("pthread reaper create main error");
        if(pid_cashier > 0){
            kill(pid_cashier, SIGTERM);
            waitpid(pid_cashier, NULL, 0);
        }
        if(pid_worker > 0){
            kill(pid_worker, SIGTERM);
            waitpid(pid_worker, NULL, 0);
        }
        loggerClose();
        detach_ipc();
        cleanup_ipc();
        exit(1);
    }
    
    res = pthread_create(&id_generator, NULL, generatorRoutine, NULL);
    if(res != 0){
        errno = res;
        perror("pthread generator create main error");
        if(pid_cashier > 0){
            kill(pid_cashier, SIGTERM);
            waitpid(pid_cashier, NULL, 0);
        }
        if(pid_worker > 0){
            kill(pid_worker, SIGTERM);
            waitpid(pid_worker, NULL, 0);
        }
        loggerClose();
        detach_ipc();
        cleanup_ipc();
        exit(1);
    }

    while(running){
        //usleep(10000);
    }
    
    pthread_join(id_generator, NULL);
    pthread_join(id_reaper, NULL);

    if(fire == 1){
        logger("[MAIN] POZAR! Ewakuuje klientow");
        signal(SIGTERM, SIG_IGN);
        kill(0, SIGTERM);
        while(waitpid(0, NULL, 0) > 0){}
        logger("[MAIN] Klienci ewakuowani");
        if(personel > 0){
            kill(-personel, SIGTERM);
        }
        waitpid(pid_cashier, NULL, 0);
        waitpid(pid_worker, NULL, 0);
    }
    else{
        signal(SIGINT, SIG_IGN);
        kill(0, SIGINT);
        while(waitpid(0, NULL, 0) > 0){}
            
        logger("[MAIN] Zamykam kase i kuchnie");
        if(personel > 0){
            kill(-personel, SIGINT);
        }
        waitpid(pid_cashier, NULL, 0);
        waitpid(pid_worker, NULL, 0);
    }

    logger("Liczba stworzonych klientow: %d", counter);
    logger("[MAIN] Koniec symulacji");
    loggerClose();

    detach_ipc();
    cleanup_ipc();
    return 0;
}