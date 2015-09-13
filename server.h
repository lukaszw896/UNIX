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
void cleanUp(packet*,game*,int,int);
void clearGameDataString(char*);


/*
 * When server process has been interrupted shared memory must be realesed.
 */
void sigint_handler(int sig);

void sigterm_handler(int);

void sigchld_handler(int);

/*
 * In case of server's death, all subprocesses must be killed.
 */
void terminate_children();

/*
 * Store child's pid in children[].
 */
void save_children_pid(int);

/*
 * Function which is a response to MOVE_DATA client message
 */
void respond_to_move_data(int clientDescriptor, packet* msg, char* tmpGameData,char* gameData, int playerType,FILE* fd,game* scrabbleGameAddress, char* playerTiles, PlayerInfo* waitingPlayer,int gameSemId );
/*
 * Function which is a response to PLAY_ANOTHER_GAME client message
 */
void respond_to_play_another_game(packet* msg,game* scrabbleGameAddress,int* dead,int playerType,int gameSemId,int clientDescriptor, int* cleanUpMemoryBool, int scrabbleGameId);
/*
 * Function which is a response to FINISH_GAME client message
 */
void respond_to_finish_the_game(packet* msg,game* scrabbleGameAddress,int* dead,int playerType,int gameSemId,int* cleanUpMemoryBool, int scrabbleGameId);

#endif