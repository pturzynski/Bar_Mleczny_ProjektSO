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
        detach_ipc();
        exit(0);
    }
}

int main(){
    BarState *bar = join_ipc();
    semlock(SEM_MEMORY);
    bar->generatorPid = getpid();
    semunlock(SEM_MEMORY);
    
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
            perror("[GENERATOR] fork error");
            exit(1);
        }
    }

    detach_ipc();
    return 0;
}
