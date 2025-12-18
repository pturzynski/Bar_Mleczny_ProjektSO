#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
 
int main(){

    srand(time(NULL));
    int size = (rand() % 3) + 1;
    int ifOrder = (rand() % 101);
    if (ifOrder <= 5){
        printf("Wychodze z baru\n");
        printf("%d ... %d\n", size, ifOrder);
        exit(0);
    }
    printf("%d ... szansa: %d\n", size, ifOrder);
    printf("Wchodze do baru\n");
    return 0;
}