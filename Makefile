all: 
	mkdir -p bin
	gcc src/main.c src/ipc.c -o bin/main
	gcc src/generator.c src/ipc.c -o bin/generator
	gcc src/client.c src/ipc.c -o bin/client
	gcc src/cashier.c src/ipc.c -o bin/cashier
	gcc src/worker.c src/ipc.c -o bin/worker

clean:
	rm -rf bin

.PHONY: all clean