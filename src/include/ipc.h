#ifndef IPC_H
#define IPC_H

#define MAX_PROCESSES 10000//maksymalna liczba procesow do stworzenia
#define KEY_SHMEM '!'
#define KEY_SEM '@'

//semafory
#define SEMNUMBER 4 //liczba semaforow
#define SEM_MEMORY 0 //semafor pamieci dzielonej
#define SEM_GENERATOR 1 //semafor dla generatora pracownikow
#define SEM_DOOR 2 //drzwi do baru, ograniczone maxClients
#define SEM_SEARCH 3 //semafor do petli szukania stolika dla klienta w client.c

//mtype komunikatow
#define MTYPE_CASHIER 1
#define MTYPE_WORKER 2

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
#include <signal.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

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
    int maxTables;
    int flagReservation;
    int flagDoubleX3; //0 - mozna podwoic, 1 - nie mozna podwoic
    int flagFire; 
    int clients;
    int maxClients;
    Table tables[];
} BarState;

typedef struct{
    long int mtype;
    int pid; 
    int payed; //0 - nie zaplacone, 1 - zaplacone 
} msgbuf;

key_t getKey(char id);
BarState* init_shmem(int x1, int x2, int x3, int x4, int maxTables);
int init_semaphores(int max_clients);
void init_queue();
BarState* init_ipc(int x1, int x2, int x3, int x4, int maxTables);
BarState* join_ipc();
void detach_ipc();
void cleanup_ipc();

void semlock(int sem_num);
void semunlock(int sem_num);
void sem_closeDoor(int sem_num, int groupSize);
void sem_openDoor(int sem_num, int groupSize);

void msgSend(int dest, msgbuf *msg);
int msgReceive(int src, msgbuf *msg, long type, int nowait);

extern int msgClient;
extern int msgCashier;
extern int msgWorker;
extern int msgStaff;
extern BarState *bar;

#endif