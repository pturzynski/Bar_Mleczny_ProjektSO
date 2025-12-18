#ifndef IPC_H
#define IPC_H

#define FTOK_KEY 155237
#define FTOK_PATH "."

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdbool.h>

typedef struct{
    int flagDoubleX3;
    int flagFire; 
    int x1, x2, x3, x4;
    int freeX1, freeX2, freeX3, freeX4;
    int clients;
    int maxClients;
} BarState;

BarState* init_ipc(int x1, int x2, int x3, int x4);
BarState* join_ipc();
void detach_ipc();
void cleanup_ipc();

#endif