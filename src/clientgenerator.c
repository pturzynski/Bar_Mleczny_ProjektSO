#include "ipc.h"
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>

int main(){
    srand(time(NULL) ^ getpid());
    while(1){
        sleep((rand() % 8 + 1));
        while (waitpid(-1, NULL, WNOHANG) > 0);
        int pid = fork();
        if(pid == 0){
            execl("./client", "klient", NULL);
            perror("[GENERATOR] execl failed");
            exit(1);
        }
        if(pid == -1){
            perror("[GENERATOR] fork error");
            exit(1);
        }
    }

    return 0;
}