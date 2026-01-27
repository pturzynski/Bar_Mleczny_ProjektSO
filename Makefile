CC = gcc

CFLAGS = -Wall -Wextra -pthread 

all: 
	mkdir -p bin
	
	$(CC) $(CFLAGS) src/main.c src/ipc.c -o main
	$(CC) $(CFLAGS) src/client.c src/ipc.c -o bin/client 
	$(CC) $(CFLAGS) src/cashier.c src/ipc.c -o bin/cashier
	$(CC) $(CFLAGS) src/worker.c src/ipc.c -o bin/worker
	$(CC) $(CFLAGS) src/manager.c src/ipc.c -o manager

clean:
	rm -rf bin
	rm -f main manager
	rm -f keyfile bar_log.txt

.PHONY: all clean