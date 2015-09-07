

CC = gcc
COMPILE_FLAGS = -Wall 
OBJ = server.o tcp_socket_util.o client.o semaphore_util.o shared_mem_util.o signals_util.o scrabble_game.o

all: server client clean

.PHONY: all clean

server: server.o tcp_socket_util.o semaphore_util.o shared_mem_util.o signals_util.o scrabble_game.o
	${CC} -o server server.o tcp_socket_util.o semaphore_util.o shared_mem_util.o signals_util.o scrabble_game.o

client: client.o tcp_socket_util.o scrabble_game.o shared_mem_util.o semaphore_util.o signals_util.o
	${CC} -o client client.o tcp_socket_util.o scrabble_game.o shared_mem_util.o semaphore_util.o signals_util.o

server.o: server.c
	${CC} -o server.o -c server.c ${COMPILE_FLAGS}

client.o: client.c
	${CC} -o client.o -c client.c ${COMPILE_FLAGS}

socket_util.o: tcp_socket_util.c
	${CC} -o tcp_socket_util.o -c tcp_socket_util.c ${COMPILE_FLAGS}
	
semaphore_util.o: semaphore_util.c
	${CC} -o semaphore_util.o -c semaphore_util.c ${COMPILE_FLAGS}

shared_mem_util.o: shared_mem_util.c
	${CC} -o shared_mem_util.o -c shared_mem_util.c ${COMPILE_FLAGS}

signals_util.o: signals_util.c
	${CC} -o signals_util.o -c signals_util.c ${COMPILE_FLAGS}

scrabble_game.o: scrabble_game.c 
	${CC} -o scrabble_game.o -c scrabble_game.c ${COMPILE_FLAGS}

clean:
	rm ${OBJ}
