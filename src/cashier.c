#include "include/ipc.h"

int income = 0;

void handle_signal(int sig){
    if(sig == SIGINT){
        logger(CASHIER_COL "[KASJER] Dochod: %d" RESET, income);
        loggerClose(); 
        _exit(0);
    }
    if(sig == SIGTERM){
        logger(CASHIER_COL "[KASJER] Klienci ewakuowani, zamykam kase i uciekam" RESET);
        logger(CASHIER_COL "[KASJER] Dochod: %d" RESET, income);
        loggerClose();
        _exit(0);
    }
}

int main(){
    loggerOpen();
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    bar = join_ipc();
    msgbuf msg;
    logger(CASHIER_COL "[KASJER] Kasa zosatała otwarta" RESET);
    int running = 1;
    while(running){
        int res = msgReceive(msgOrder, &msg, MTYPE_CASHIER);
        if(res == -1){
            continue;
        }
        if(res == -2){
            break;
        }
        logger(CASHIER_COL "[KASJER] Klient %d zaplacil %d za zamowienie" RESET, msg.pid, msg.price);
        income += msg.price;
        msg.mtype = msg.pid;
        msgSend(msgOrder, &msg);
    }
    logger(CASHIER_COL "[KASJER] Kasa zamknieta" RESET);
    loggerClose();
    
    detach_ipc();
    return 0;
}