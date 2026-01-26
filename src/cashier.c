#include "include/ipc.h"

int income = 0;

void handle_signal(int sig){
    if(sig == SIGINT){
        logger(CASHIER_COL "[KASJER] Dochod: %d" RESET, income);    
        _exit(0);
    }
    if(sig == SIGTERM){
        logger(CASHIER_COL "[KASJER] Klienci ewakuowani, zamykam kase i uciekam" RESET);
        logger(CASHIER_COL "[KASJER] Dochod: %d" RESET, income);
        _exit(0);
    }
}

int main(){
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
        int res = msgReceive(msgCashier, &msg, MTYPE_CASHIER);
        if(res == -1){
            continue;
        }
        if(res == -2){
            break;
        }
        logger(CASHIER_COL "[KASJER] Klient %d zaplacil %d za zamowienie" RESET, msg.pid, msg.price);
        income += msg.price;
        msg.mtype = msg.pid;
        msgSend(msgClient, &msg);
    }
    logger(CASHIER_COL "[KASJER] Kasa zamknieta" RESET);
    detach_ipc();
    return 0;
}