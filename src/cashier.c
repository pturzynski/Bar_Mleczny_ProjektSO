#include "include/ipc.h"

volatile sig_atomic_t running = 1; 

void handle_signals(int sig){
    if(sig == SIGQUIT){
        printf(CASHIER_COL "[KASJER] POZAR ! EWAKUACJA !\n" RESET);
        detach_ipc();
        exit(0);
    }
}

int main(){
    BarState *bar = join_ipc();
    msgbuf msg;

    struct sigaction sa;
    sa.sa_handler = handle_signals;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGQUIT, &sa, NULL);
    
    semlock(SEM_MEMORY);
    bar->cashierPid = getpid();
    semunlock(SEM_MEMORY);
    
    printf(CASHIER_COL "[KASJER] Kasa zosatała otwarta\n" RESET);
    while(running){
        msgReceive(msgCashier, &msg, MTYPE_CASHIER);
        printf(CASHIER_COL "[KASJER] Klient %d zaplacil za zamowienie\n" RESET, msg.pid);
        msg.mtype = msg.pid;
        msg.payed = 1;
        msgSend(msgClient, &msg);
    }

    printf(CASHIER_COL "[KASJER] Kasa zamknieta\n" RESET);
    detach_ipc();
    return 0;
}