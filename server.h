#ifndef SERVER_H
#define SERVER_H

#define _GNU_SOURCE

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Custom includes */
#include "settings.h"
#include "signals_util.h"
#include "tcp_socket_util.h"
#include "semaphore_util.h"
#include "shared_mem_util.h"
#include "scrabble_game.h"

#define GAME_DATA_ENTRY 3000

/*
 * When SIGINT is catched, this variable will tell main loop to stop.
 */
volatile sig_atomic_t g_doWork = 1;

volatile sig_atomic_t g_flag = 0;


/*       Host to network / network to host      */
/*
 * For children's pids.
 */
volatile sig_atomic_t children[MAX_CHILDREN];

typedef struct
{
    int status;
    int shmId;
    int semId;
    FILE* fd;
    char fileName[GAME_DATA_ENTRY];
} PlayerInfo;

/*
 * Each client after accepting, gets his own process and permorms this function.
 */
void handle_client(int, int, PlayerInfo*,struct sockaddr_in);
/*
 * Cleans up game memory
 */
void cleanUp(packet*,game*,int,int);

void clearGameDataString(char*);

void sigint_handler(int sig);

void sigterm_handler(int);

void sigchld_handler(int);

/*
 * Function which is a response to MOVE_DATA client message
 */
void respond_to_move_data(int clientDescriptor, packet* msg, char* tmpGameData,char* gameData, int playerType,FILE* fd,game* scrabbleGameAddress, char* playerTiles, char* fileName,int gameSemId );
/*
 * Function which is a response to PLAY_ANOTHER_GAME client message
 */
void respond_to_play_another_game(packet* msg,game* scrabbleGameAddress,int* dead,int playerType,int gameSemId,int clientDescriptor, int* cleanUpMemoryBool, int scrabbleGameId);
/*
 * Function which is a response to FINISH_GAME client message
 */
void respond_to_finish_the_game(packet* msg,game* scrabbleGameAddress,int* dead,int playerType,int gameSemId,int* cleanUpMemoryBool, int scrabbleGameId);
/*
 * Function which do stuff after client is disconnected
 */
void client_is_disconnected(int clientDescriptor,game* scrabbleGameAddress,int* isThisClientDisconnected, int playerType, int gameSemId,int* dead);

ssize_t bulk_fwrite(FILE* fd, char* buf);
/*
 * open file "fd" with name "fileName" and write "gameData" string to it.
 */
void write_to_file(FILE* fd,char* fileName, char* gameData);

#endif