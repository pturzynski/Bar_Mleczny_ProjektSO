all: 
	mkdir -p bin
	gcc src/main.c src/ipc.c -o main
	gcc src/generator.c src/ipc.c -o bin/generator
	gcc src/client.c src/ipc.c -o bin/client
	gcc src/cashier.c src/ipc.c -o bin/cashier
	gcc src/worker.c src/ipc.c -o bin/worker
	gcc src/manager.c src/ipc.c -o manager

clean:
	rm -rf bin

.PHONY: all clean