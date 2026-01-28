#ifndef IPC_H
#define IPC_H

#define MAX_PROCESSES 8000//maksymalna liczba procesow do stworzenia
#define KEY_SHMEM '!'
#define KEY_SEM '@'

//semafory
#define SEMNUMBER 6 //liczba semaforow
#define SEM_MEMORY 0 //semafor pamieci dzielonej
#define SEM_GENERATOR 1 //semafor dla generatora
#define SEM_DOOR 2 //drzwi do baru, ograniczone maxClients
#define SEM_SEARCH 3 //semafor do petli szukania stolika dla klienta w client.c
#define SEM_ORDER 4 //semafor do kolejki komunikatow msgOrder
#define SEM_FOOD 5 //semafor do kolejki komunikatow msgFood

//mtype komunikatow
#define MTYPE_CASHIER 1 //mtype do komunikacji z kasjerem
#define MTYPE_WORKER 2 //mtype do komunikacji z pracownikiem
#define MTYPE_RESERVATION 3 //mtype do rezerwacji stolikow (menadzer -> worker)

//informacje o tym jak proces sie zakonczyl, do logow klienta
#define EXIT_EATEN 10  //zjadl i wyszedl
#define EXIT_NOORDER 11  //nie zamawia
#define EXIT_NOTABLE 12  //caly bar zarezerwowany
#define EXIT_FRUSTRATED 13  //frustracja

//kolorki
#define RESET "\033[0m" //reset
#define CASHIER_COL "\033[34m" //niebieski
#define CLIENT_COL "\033[33m" //zolty
#define WORKER_COL "\033[32m" //zielony

#define LOG_FILE "bar_log.txt" //plik logow

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
#include <stdarg.h>
#include <sys/file.h>


/*
    Struktura stolikow, tez jest czescia pamieci dzielonej
*/
typedef struct{
    int id;
    int capacity;
    int whoSits; 
    int freeSlots; 
    int isReserved;
} Table;

/*
    Struktura pamieci dzielonej
*/
typedef struct{
    int x1, x2, x3, x4; //stoliki kolejno 1, 2, 3 i 4 osobowe
    int allTables; //Wszystkie stoliki w barze
    int maxTables; //Maksymalnie ile moze by stolikow w barze, potrzebne do podwojenia stolikow x3
    int clients; //aktualna liczba klientow w barze
    int maxClients; //maksymalna liczba klientow w barze
    pid_t workerPid; //pid pracownika
    pid_t mainPid;  //pid glownego programu
    pid_t managerPid;  //pid menadzera
    Table tables[]; //Elastyczna tablica stolikow 
} BarState;


/*
    Struktura kolejki komunikatow
*/
typedef struct{
    long int mtype;
    int pid; 
    int price;
    int tableType; //do rezerwacji
    int count; //do rezerwacji
    int success; //potwierdzenie dla menadzera jak zakonczyla sie operacja
} msgbuf;

/*
    Funkcje do poprawnej inicjalizacji semaforow, pamieci dzielonej, kolejek komunikatow
*/
key_t getKey(char id);
BarState* init_shmem(int x1, int x2, int x3, int x4, int maxTables);
int init_semaphores();
void init_queue();
BarState* init_ipc(int x1, int x2, int x3, int x4, int maxTables);
BarState* join_ipc();
void detach_ipc();
void cleanup_ipc();

/*
    Opuszcza semafor (P)
    Blokuje proces gdy wartosc semafora wynosi 0
    int undo, jesli 1 to jest flaga SEM_UNDO jesli jest 0 tej flagi nie ma
    return -1 jest przerwano sygnalem
    return -2 w przypadku krytycznym 
*/
int semlock(int sem_num, int undo);
/*
    Podnosi semafor (V)
    Budzi czekajace procesy
    int undo, jesli 1 to jest flaga SEM_UNDo, jesli 0 to tej flagi nie ma
    return 0 w przypadku sukcesu
*/
int semunlock(int sem_num, int undo);

/*
    Opuszcza semafor (P) o liczbe groupSize
*/
int sem_closeDoor(int sem_num, int groupSize, int undo);
/*
    Podnosi semafor (V) o liczbe groupSize
*/
int sem_openDoor(int sem_num, int groupSize, int undo);

/*
    Sprawdza czy ktos czeka na semaforze
    jesli tak robi semunlock() V
    jesli nie return 0
*/
int sem_wakeOne(int sem_num); //budzi czekajacych na semaforze

/*
    Funkcje do obslugi kolejki komunikatow
    msgsnd i msgrcv z obslugami bledow
    return -1 jesli przerwano sygnalem
    return -2 w przypadku krytycznym
*/
int msgSend(int dest, msgbuf *msg);
int msgReceive(int src, msgbuf *msg, long type);


extern int msgOrder;
extern int msgFood;
extern int msgStaff;
extern BarState *bar;
extern int loggerFile;

/*
    Funkcje do zapisu logow do pliku
*/
void loggerOpen();
void loggerClose();
void logger(const char *format, ...);

#endif