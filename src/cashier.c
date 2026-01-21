#include "include/ipc.h"

void handle_signals(int sig){
    if(sig == SIGQUIT){
        logger(CASHIER_COL "[KASJER] POZAR ! EWAKUACJA !" RESET);
        detach_ipc();
        exit(0);
    }
    if(sig == SIGINT){
        logger(CASHIER_COL "[KASJER] Zamykam kase" RESET);
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
    sigaction(SIGINT, &sa, NULL);
    
    int running = 1;
    logger(CASHIER_COL "[KASJER] Kasa zosatała otwarta" RESET);
    while(running){
        int res = msgReceive(msgCashier, &msg, MTYPE_CASHIER);
        if(res == -1){
            continue;
        }
        if(res == -2){
            break;
        }
        logger(CASHIER_COL "[KASJER] Klient %d zaplacil za zamowienie" RESET, msg.pid);
        msg.mtype = msg.pid;
        msg.payed = 1;
        msgSend(msgClient, &msg);
    }

    printf(CASHIER_COL "[KASJER] Kasa zamknieta" RESET);
    detach_ipc();
    return 0;
}