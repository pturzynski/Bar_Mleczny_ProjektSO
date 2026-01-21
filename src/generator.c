#include "include/ipc.h"
#include <signal.h>

volatile sig_atomic_t running = 1;

void handle_sigchld(int sig){
    if(sig == SIGCHLD){
        while(waitpid(-1, NULL, WNOHANG) > 0);
    }
}

void handle_signal(int sig){
    if(sig == SIGINT || sig == SIGTERM){
        detach_ipc();
        exit(0);
    }
    if(sig == SIGQUIT){
        running = 0;
    }
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
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    
    while(running){
        if(semlock(SEM_GENERATOR) == -1){
            continue;
        }

        if(!running){
            semunlock(SEM_GENERATOR);
            break;
        }

        int pid = fork();
        if (pid == 0){
            execl("bin/client", "klient", NULL);
            perror("[GENERATOR] execl failed");
            detach_ipc();
            exit(1);
        }
        else if (pid == -1){
            semunlock(SEM_GENERATOR);
            perror("[GENERATOR] fork error");
            detach_ipc();
            exit(1);
        }
    }

    while(wait(NULL) > 0);
    detach_ipc();
    return 0;
}
