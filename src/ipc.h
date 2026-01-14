#ifndef IPC_H
#define IPC_H

#define FTOK_KEY 155237 
#define FTOK_PATH "."
#define MAX_PROCESSES 500

#define SEMNUMBER 5 //liczba semaforow
#define SEM_MEMORY 0 //semafor pamieci dzielonej
#define SEM_CASHIER 1 //semafor kasjera
#define SEM_WORKER 2 //semafor pracownika
#define SEM_GENERATOR 3 //semafor dla generatora pracownikow
#define SEM_MSG_LIMIT 4 //limit dla kolejki komunikatow

#define MTYPE_CASHIER 1
#define MTYPE_WORKER 2

#define WORKER_FOOD 1 //odbieranie jedzenia od pracownika
#define WORKER_CLEAN 2 //pracownik sprzata po kliencie

#define RESET   "\033[0m"
#define CASHIER_COL "\033[34m" //niebieski
#define CLIENT_COL "\033[33m" //zolty
#define WORKER_COL "\033[32m" //zielony

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <time.h>
#include <errno.h>

extern int keep_running;

typedef struct{
    int id;
    int capacity;
    int whoSits; 
    int freeSlots; 
    int isReserved;
} Table;

typedef struct{
    int x1, x2, x3, x4;
    int allTables;
    int flagReservation;
    int flagDoubleX3;
    int flagFire; 
    int clients;
    int maxClients;
    Table tables[];
} BarState;

BarState* init_ipc(int x1, int x2, int x3, int x4, int maxTables);
BarState* join_ipc();
void detach_ipc();
void cleanup_ipc();
void semlock(int sem_num);
void semunlock(int sem_num);

typedef struct{
    long int mtype;
    int pid;
    int groupSize;
    int order; //0 - nie ma miejsca, nie zamowimy 1 - jest miejsce, zamowione, idziemy odebrac danie
    int tableId;
    int action;
} msgbuf;

void msgSend(msgbuf *m);
int msgReceive(msgbuf *m, long int mtype);

#endif