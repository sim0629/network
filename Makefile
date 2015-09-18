# 심규민 2009-11744

CC = gcc
INC = -I.
FLAGS = -W -Wall -g
TARGETS = server client

all: $(TARGETS)

server: server.o socket.o
	$(CC) $^ -o $@ -pthread

client: client.o socket.o
	$(CC) $^ -o $@ -pthread

server.o: server.c
	$(CC) -c $(FLAGS) $(INC) $< -o $@ -pthread

client.o: client.c
	$(CC) -c $(FLAGS) $(INC) $< -o $@ -pthread

socket.o: socket.c
	$(CC) -c $(FLAGS) $(INC) $< -o $@ -pthread

.PHONY: clean
clean:
	-rm -f *.o $(TARGETS)
