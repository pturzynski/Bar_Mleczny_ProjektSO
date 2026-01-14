#include "ipc.h"

int keep_running = 1;

void handle_sigint(int sig) {
    keep_running = 0;
}

void zombieClean(int sig){
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(){
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
    signal(SIGCHLD, zombieClean);
    srand(time(NULL) ^ getpid());
    BarState *bar = join_ipc();
    while(keep_running){
        semlock(SEM_GENERATOR);
        int pid = fork();
        if (pid == 0){
            execl("./client", "klient", NULL);
            perror("[GENERATOR] execl failed");
            semunlock(SEM_GENERATOR);
            exit(1);
        }
        else if (pid == -1){
            perror("[GENERATOR] fork error");
            semunlock(SEM_GENERATOR);
            exit(1);
        }
        //sleep(1);
    }
    while (wait(NULL) > 0);
    detach_ipc();
    return 0;
}