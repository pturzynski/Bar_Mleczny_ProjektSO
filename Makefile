all: 
	mkdir -p bin
	gcc -Wall -Wextra -pthread src/main.c src/ipc.c -o main
	gcc -Wall -Wextra -pthread src/client.c src/ipc.c -o bin/client 
	gcc -Wall -Wextra src/cashier.c src/ipc.c -o bin/cashier
	gcc -Wall -Wextra src/worker.c src/ipc.c -o bin/worker
	gcc -Wall -Wextra src/manager.c src/ipc.c -o manager

clean:
	rm -rf bin

.PHONY: all clean