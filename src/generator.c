#include "ipc.h"
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

int keep_running = 1;

void zombieClean(int sig){
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(){
    signal(SIGCHLD, zombieClean);
    srand(time(NULL) ^ getpid());
    while(keep_running){
        int pid = fork();
        if (pid == 0){
            execl("./client", "klient", NULL);
            perror("[GENERATOR] execl failed");
            exit(1);
        }
        if (pid == -1){
            perror("[GENERATOR] fork error");
            exit(1);
        }
        sleep(2);
        //sleep((rand() % 2 + 1));
    }
    return 0;
}