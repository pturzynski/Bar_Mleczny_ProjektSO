#include "include/ipc.h"
#include <signal.h>

volatile sig_atomic_t running = 1; 

void handle_sigchld(int sig){
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void handle_signals(int sig){
    running = 0;
}

int main(){
    BarState *bar = join_ipc();
    struct sigaction sa;
    
    //sigclhd
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    //sigterm sigint
    sa.sa_handler = handle_signals;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    while(running){
        semlock(SEM_GENERATOR);

        if(!running){
            semunlock(SEM_GENERATOR);
            break;
        }

        int pid = fork();
        if (pid == 0){
            execl("bin/client", "klient", NULL);
            perror("[GENERATOR] execl failed");
            exit(1);
        }
        else if (pid == -1){
            semunlock(SEM_GENERATOR);
            continue;
        }
    }

    detach_ipc();
    return 0;
}