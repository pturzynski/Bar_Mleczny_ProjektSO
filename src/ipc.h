#ifndef IPC_H
#define IPC_H

#define FTOK_KEY 155237
#define FTOK_PATH "."
#define SEM_KEY 237155

#define SEMNUMBER 2
#define SEM_MEMORY 0
#define SEM_CASHIER 1

#define MTYPE_CASHIER 1

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <time.h>
#include <errno.h>

extern int keep_running;

typedef struct{
    int id;
    int capacity;
    int groupSize;
    int freeSlots;
    int isReserved;
} Table;

typedef struct{
    int x1, x2, x3, x4;
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
} msgbuf;

void msgSend(msgbuf *m);
int msgReceive(msgbuf *m, long int mtype);

#endif