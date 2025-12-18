#include "ipc.h"

int main(){
    int x1, x2, x3, x4;
    printf("Podaj liczbe stolikow kolejno: 1-os, 2-os, 3-os, 4-os (oddziel spacja)\n");
    printf("PRZYKLAD: 1 2 3 4\n");
    scanf("%d %d %d %d", &x1, &x2, &x3, &x4);

    BarState *bar = init_ipc(x1, x2, x3, x4);
    printf("Ilosc stolikow %d , %d , %d , %d\n", bar->x1, bar->x2, bar->x3, bar->x4);
    detach_ipc();
    cleanup_ipc();
    return 0;
}