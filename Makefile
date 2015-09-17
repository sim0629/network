CC = gcc
INC = -I.
FLAGS = -W -Wall -g
TARGETS = server client

all: $(TARGETS)

server: server.o socket.o
	$(CC) $^ -o $@ -lrt

client: client.o socket.o
	$(CC) $^ -o $@ -lrt

server.o: server.c
	$(CC) -c $(FLAGS) $(INC) $< -o $@ -lrt

client.o: client.c
	$(CC) -c $(FLAGS) $(INC) $< -o $@ -lrt

socket.o: socket.c
	$(CC) -c $(FLAGS) $(INC) $< -o $@ -lrt

.PHONY: clean
clean:
	-rm -f *.o $(TARGETS)
