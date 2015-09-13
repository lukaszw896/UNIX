#define _GNU_SOURCE 
/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

/* Custom includes */
#include "settings.h"
#include "signals_util.h"
#include "tcp_socket_util.h"
#include "semaphore_util.h"
#include "shared_mem_util.h"
#include "scrabble_game.h"

/*****	tcp  *******/
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <fcntl.h>

#define BACKLOG 3
#define GAME_DATA_ENTRY 3000
#define LOG_FILE_NAME "LogFile.dat"
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
void cleanUp(packet*,packet*,game*,int*,int);
void clearGameDataString(char*);

ssize_t bulk_fwrite(FILE* fd, char *buf) {
    int c;
    size_t count = strlen(buf);
    size_t len = 0;
    do {
        c = TEMP_FAILURE_RETRY(fwrite(buf, sizeof(char), strlen(buf), fd));
        if (c < 0) return c;
        buf += c;
        len += c;
        count -= c;
    } while(count > 0);
    return len ;
}

/*
 * When server process has been interrupted shared memory must be realesed.
 */
void sigint_handler(int sig);

/*
 * TODO
 */
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

void respond_to_move_data(int clientDescriptor, packet* msg, char* tmpGameData,char* gameData, int playerType,FILE* fd,game* scrabbleGameAddress, char* playerTiles, PlayerInfo* waitingPlayer,int gameSemId ){
    int u;
    printf("[Client %d] New letter: %c\n", clientDescriptor, msg->letter);
    snprintf(tmpGameData,GAME_DATA_ENTRY,"%s[Client %d] New letter: %c\n", gameData, clientDescriptor, msg->letter);
    snprintf(gameData,GAME_DATA_ENTRY,"%s", tmpGameData);

    printf("[Client %d] Coordinates: (%d,%d)\n", clientDescriptor, msg->x_coord, msg->y_coord);
    snprintf(tmpGameData,GAME_DATA_ENTRY,"%s[Client %d] Coordinates: (%d,%d)\n", gameData,clientDescriptor, msg->x_coord, msg->y_coord);
    snprintf(gameData,GAME_DATA_ENTRY,"%s", tmpGameData);

    printf("[Client %d] Tiles_returned: %c %c %c %c %c\n", clientDescriptor, msg->tiles[0],
           msg->tiles[1], msg->tiles[2], msg->tiles[3], msg->tiles[4]);
    snprintf(tmpGameData,GAME_DATA_ENTRY,"%s[Client %d] Tiles_returned: %c %c %c %c %c\n", gameData, clientDescriptor, msg->tiles[0],
             msg->tiles[1], msg->tiles[2], msg->tiles[3], msg->tiles[4]);
    snprintf(gameData,GAME_DATA_ENTRY,"%s", tmpGameData);

    /* Update shared memory */
    scrabbleGameAddress->gameBoard[msg->x_coord][msg->y_coord] = msg->letter;
    scrabbleGameAddress->p1Points = msg->p1Points;
    scrabbleGameAddress->p2Points = msg->p2Points;

    /* Delete used tile */
    for (u = 0; u < 5; u++)
        if (playerTiles[u] == msg->letter) {
            playerTiles[u] = 'x';
            break;
        }
    printf("[Client %d] Tiles_current:  %c %c %c %c %c\n", clientDescriptor, playerTiles[0],
           playerTiles[1], playerTiles[2], playerTiles[3], playerTiles[4]);
    scrabble_game_print_available_tiles(scrabbleGameAddress->avTiles, 25);

    fd =fopen(waitingPlayer->fileName, "a");
    if(bulk_fwrite(fd,gameData)<0) ERR("write:");
    clearGameDataString(gameData);
    fclose(fd);

    if (playerType == SECOND) {
        semaphore_unlock(gameSemId, 1, 0);
    } else {
        semaphore_unlock(gameSemId, 2, 0);
    }
}

/*
 * Core of the server.
 */
int main(void)
{
	
    int serverSocket, clientSocket, pid, stackSem, waitingPlayerSocketId;

    
    PlayerInfo* waitingPlayerSocketAddress = NULL;
    socklen_t t;
    struct sockaddr_in local, remote;

	/* Waiting player socket in shared memory */
	shared_mem_init(&waitingPlayerSocketId,sizeof(int),'E');
	waitingPlayerSocketAddress = (PlayerInfo*)shared_mem_attach(waitingPlayerSocketId);
	waitingPlayerSocketAddress->status = NO_PLAYER;
	
	/* Stack semaphore */
	semaphore_init(&stackSem, 'E', 1);

	/* Server socket */
	tcp_socket_init_unix(&serverSocket, &local, SOCK_PATH);
	tcp_socket_bind(&serverSocket, &local);
	tcp_socket_listen(&serverSocket, INCOMMING_CONN);
	
	/* Set signals handlers */
	if(sethandler(sigint_handler,SIGINT)) ERR("Setting SIGINT:");
	if(sethandler(sigchld_handler,SIGCHLD)) ERR("Setting SIGCHLD:");
	if(sethandler(sigterm_handler,SIGTERM)) ERR("Setting SIGTERM:");
    while(g_doWork) {
        printf("Waiting for a clients...\n");
        t = sizeof(remote);
        if ((clientSocket = TEMP_FAILURE_RETRY(accept(serverSocket, (struct sockaddr *)&remote, &t))) <0) {
            perror("accept");
			break;
        }


        printf("Connected.\n");

		switch(pid = fork())
		{
			case 0:
				/* Set handler */
				//if(sethandler(sigterm_handler,SIGTERM)) ERR("Setting SIGTERM:");
				/* Save child's pid for parent */
				save_children_pid(getpid());
				handle_client(clientSocket, stackSem, waitingPlayerSocketAddress,remote);
				printf("Client with pid: %d disconnected.\n", getpid());
				printf("Waiting for a clients'...\n");
				return EXIT_SUCCESS;
			default:
				if(pid < 0)
				{
					 perror("ERROR on fork");
					 exit(1);					
				}
				
		}
        
    }
	//terminate_children();
    terminate_children();
	printf("MAIN %d\n",getpid());
	semaphore_remove(stackSem);
	shared_mem_detach((char*)waitingPlayerSocketAddress);
	shared_mem_delete(waitingPlayerSocketId);
	
	/* Wait for children. */
	for(;;){
		pid=wait(NULL);
		if(pid<0)
			switch (errno){
				case ECHILD:
					return EXIT_SUCCESS;
				case EINTR:
					continue;
				default:
					perror("wait:");
					exit(EXIT_FAILURE);
			}
	}
}

void handle_client(int clientDescriptor, int stackSem, PlayerInfo* waitingPlayer,struct sockaddr_in clientAddress) {

    packet* msg = malloc(sizeof (packet));
    packet* msg2 = malloc(sizeof (packet));
    game* scrabbleGameAddress = NULL;
    FILE* fd = NULL;
    char playerTiles[5] = {'x', 'x', 'x', 'x', 'x'};
    
    char fileName[GAME_DATA_ENTRY];
    char gameData[GAME_DATA_ENTRY];
    char tmpGameData[GAME_DATA_ENTRY];
    int scrabbleGameId, gameSemId;

    int isThisClientDisconnected;

    //variable indicating whether client server process should clean up
    int cleanUpMemoryBool;

    /* Control variable for connection lost. */
    int dead = 0;
    /* To discriminate player, which starts the game */
    int playerType;
    int u, v;

    while (g_doWork) {
        msg = malloc(sizeof (packet));
        msg2 = malloc(sizeof (packet));
        scrabbleGameAddress = NULL;
        //playerTiles = {'x', 'x', 'x', 'x', 'x'};
        playerTiles[0] = 'x';
        playerTiles[1] = 'x';
        playerTiles[2] = 'x';
        playerTiles[3] = 'x';
        playerTiles[4] = 'x';
        isThisClientDisconnected=0;
        cleanUpMemoryBool = 1;
        dead = 0;
        printf("BEGGINING OF EVERYTHING\n");
        semaphore_lock(stackSem, 0, 0);
        printf("[CLIENT %d] in charge of queue's Shared memory %d\n", clientDescriptor, stackSem);

        /***********************************
         * 		NO PLAYER AWAITS
         * ********************************/
        if (waitingPlayer->status == NO_PLAYER && g_doWork) {
            
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);

            snprintf(fileName,GAME_DATA_ENTRY,"%d%d%d%d%d%d.dat", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

            //writing ip and socket info to a file by first player in the game
            fd =fopen(fileName, "a");
            snprintf(gameData,GAME_DATA_ENTRY,"First player IP: %d and Socket:%d\n", (int)clientAddress.sin_addr.s_addr,(int)clientAddress.sin_port);
            if(bulk_fwrite(fd,gameData)<0) ERR("write:");
            clearGameDataString(gameData);

            if(0 != fclose(fd)){
               ERR("fclose");
            }

            /* Create game  in shared memory for future play */


            shared_mem_init(&scrabbleGameId, sizeof (game), 'G');
            scrabbleGameAddress = (game*) shared_mem_attach(scrabbleGameId);
            game newGame;
            newGame.status = DISCONNECTED;
            scrabble_game_attach_tiles(newGame.avTiles);
            *scrabbleGameAddress = newGame;
            scrabbleGameAddress->p1Points = 0;
            scrabbleGameAddress->p2Points = 0;
            scrabbleGameAddress->didClientDisconnect = 0;
            scrabbleGameAddress->didGameEnd = 0;

            scrabble_game_blank(scrabbleGameAddress);
            printf("Game has been initialized.\n");
            scrabble_game_print_available_tiles(scrabbleGameAddress->avTiles, 25);

            /* Create semaphore to control shared memory */
            semaphore_init(&gameSemId, getpid(), 3);

            /*lock second game loop semaphore*/
            semaphore_lock(gameSemId, 2, 0);

            /* Inform client about changes */
            msg->msg = NO_PLAYER;
            tcp_socket_send_packet(clientDescriptor, msg);

            /* Save info for future player */
            PlayerInfo playerInfo;
            playerInfo.shmId = scrabbleGameId;
            playerInfo.semId = gameSemId;
            playerInfo.fd = fd;
            strcpy(playerInfo.fileName,fileName);

            *waitingPlayer = playerInfo;

            /* Unlock queue's shared memory */
            semaphore_unlock(stackSem, 0, 0);


            while (g_doWork) {
                /* Check if client is still connected to the process. */
                if (recv(clientDescriptor, msg, sizeof (msg), MSG_DONTWAIT) == 0 ) {
                    /* Remove data from the queue. */
                    semaphore_lock(stackSem, 0, 0);
                    waitingPlayer->status = NO_PLAYER;
                    waitingPlayer->semId = NONE;
                    waitingPlayer->shmId = NONE;
                    semaphore_unlock(stackSem, 0, 0);

                    printf("[CLIENT %d] disconnected :/\n", clientDescriptor);
                    dead = 1;
                    g_doWork=0;
                    isThisClientDisconnected = 1;
                    break;

                }
                /* Check if someone else connected to the game */
                semaphore_lock(gameSemId, 0, 0);
                if (scrabbleGameAddress->status == CONNECTED) {
                    /* Inform client that the game is about to start. */
                    msg->msg = PAIR_MATCH;
                    tcp_socket_send_packet(clientDescriptor, msg);
                    semaphore_unlock(gameSemId, 0, 0);
                    break;
                }
                semaphore_unlock(gameSemId, 0, 0);
            };
            playerType = FIRST;

        }/*******************************************
		 * 		  PLAYER IS WAITING FOR GAME
		 ******************************************/

        else if (g_doWork) {
            // Retrieve semaphore's and shared memory's ids.
            gameSemId = waitingPlayer->semId;
            scrabbleGameId = waitingPlayer->shmId;

            // Attach to the shared memory address.
            scrabbleGameAddress = (game*) shared_mem_attach(scrabbleGameId);

            /* Indicate that waiting queue is empty. */
            waitingPlayer->status = NO_PLAYER;
            waitingPlayer->semId = NONE;
            waitingPlayer->shmId = NONE;
            waitingPlayer->fd = NULL;
            
            /* Unlock queue's shared memory */
            semaphore_unlock(stackSem, 0, 0);

            /* Inform game that second player has connected. */
            semaphore_lock(gameSemId, 0, 0);
            scrabbleGameAddress->status = CONNECTED;

            //writing ip and socket info to a file by second player
            fd =fopen(waitingPlayer->fileName, "a");
            snprintf(gameData,GAME_DATA_ENTRY,"Second player IP: %d and Socket:%d\n", (int)clientAddress.sin_addr.s_addr,(int)clientAddress.sin_port);
            if(bulk_fwrite(fd,gameData)<0) ERR("write:");
            clearGameDataString(gameData);

            if(0 != fclose(fd)){
                ERR("fclose");
            }
            
            /* Since this player moves second, send empty game board and tiles*/
            msg->msg = INFO;
            msg->letter = NONE;
            msg->x_coord = NONE;
            msg->y_coord = NONE;
            for (u = 0; u < 5; u++)
                for (v = 0; v < 5; v++)
                    msg->currentBoard[u][v] = scrabbleGameAddress->gameBoard[u][v];
            tcp_socket_send_packet(clientDescriptor, msg);

            semaphore_unlock(gameSemId, 0, 0);

            playerType = SECOND;
        }

        /*****************************************************
         * 					START GAME
         * **************************************************/

        printf("[Client %d] Game begins.\n", clientDescriptor);

        /* Main game loop */
        while (1 && (!dead) && g_doWork) {
            printf("BEGINNING OF GAME LOOP \n");
            if (playerType == FIRST) {
                semaphore_lock(gameSemId, 1, 0);
            } else {
                semaphore_lock(gameSemId, 2, 0);
            }
            //semaphore_lock(gameSemId,0,0);
            printf("[Client %d] in charge of game semaphore\n", clientDescriptor);

            if (scrabbleGameAddress->didClientDisconnect == 1) {
                printf("[Client %d]DISCONNECT!!!!!!!!\n", clientDescriptor);
                msg->msg = DISCONNECTED;
                tcp_socket_send_packet(clientDescriptor, msg);
                
            } else {


                /* Request new move from player */
                msg->msg = REQUEST_MOVE;
                msg->letter = NONE;
                msg->x_coord = NONE;
                msg->y_coord = NONE;
                msg->p1Points = scrabbleGameAddress->p1Points;
                msg->p2Points = scrabbleGameAddress->p2Points;
                msg->playerType = playerType;

                /* Game board info. */
                for (u = 0; u < 5; u++)
                    for (v = 0; v < 5; v++)
                        msg->currentBoard[u][v] = scrabbleGameAddress->gameBoard[u][v];

                /* Tiles info. */
                for (u = 0; u < 5; u++)
                    if (playerTiles[u] == 'x') {
                        char c = scrabble_game_get_random_tile(scrabbleGameAddress->avTiles);
                        msg->tiles[u] = c;
                        playerTiles[u] = c;
                    }
                snprintf(gameData,GAME_DATA_ENTRY,"[Client %d] Tiles_sent:     %c %c %c %c %c\n", clientDescriptor, msg->tiles[0],
                         msg->tiles[1], msg->tiles[2], msg->tiles[3], msg->tiles[4]);
                printf("[Client %d] Tiles_sent:     %c %c %c %c %c\n", clientDescriptor, msg->tiles[0],
                        msg->tiles[1], msg->tiles[2], msg->tiles[3], msg->tiles[4]);

                snprintf(tmpGameData,GAME_DATA_ENTRY,"%s[Client %d] P1 points: %d P2 points: %d\n", gameData, clientDescriptor, msg->p1Points, msg->p2Points);
                snprintf(gameData,GAME_DATA_ENTRY,"%s", tmpGameData);
                printf("[Client %d] P1 points: %d P2 points: %d\n", clientDescriptor, msg->p1Points, msg->p2Points);
                tcp_socket_send_packet(clientDescriptor, msg);
            }
            /* Get new move. */
            int t;

             /************************************************
             * 		 HANDLING DISCONNECTED CLIENT           
             * **********************************************/
            if ((t = tcp_socket_read_packet(clientDescriptor, msg)) <= 0) {
                printf("Client %d Disconnected\n", clientDescriptor);
                // if another player client was not disconnected than we don't want to clear anything yet
                if(scrabbleGameAddress->didClientDisconnect == 0) {
                    scrabbleGameAddress->didClientDisconnect = 1;
                    isThisClientDisconnected = 0;
                }
                else{
                    isThisClientDisconnected = 1;
                }

                //cleanUpMemoryBool=0;

                if (playerType == SECOND) {
                    semaphore_unlock(gameSemId, 1, 0);
                } else {
                    semaphore_unlock(gameSemId, 2, 0);
                }
                dead = 1;

                g_doWork = 0;
            }  
             else {
                if (msg->msg == MOVE_DATA) {
                    printf("[Client %d] New letter: %c\n", clientDescriptor, msg->letter);
                    snprintf(tmpGameData,GAME_DATA_ENTRY,"%s[Client %d] New letter: %c\n", gameData, clientDescriptor, msg->letter);
                    snprintf(gameData,GAME_DATA_ENTRY,"%s", tmpGameData);

                    printf("[Client %d] Coordinates: (%d,%d)\n", clientDescriptor, msg->x_coord, msg->y_coord);
                    snprintf(tmpGameData,GAME_DATA_ENTRY,"%s[Client %d] Coordinates: (%d,%d)\n", gameData,clientDescriptor, msg->x_coord, msg->y_coord);
                    snprintf(gameData,GAME_DATA_ENTRY,"%s", tmpGameData);

                    printf("[Client %d] Tiles_returned: %c %c %c %c %c\n", clientDescriptor, msg->tiles[0],
                            msg->tiles[1], msg->tiles[2], msg->tiles[3], msg->tiles[4]);
                    snprintf(tmpGameData,GAME_DATA_ENTRY,"%s[Client %d] Tiles_returned: %c %c %c %c %c\n", gameData, clientDescriptor, msg->tiles[0],
                             msg->tiles[1], msg->tiles[2], msg->tiles[3], msg->tiles[4]);
                    snprintf(gameData,GAME_DATA_ENTRY,"%s", tmpGameData);

                    /* Update shared memory */
                    scrabbleGameAddress->gameBoard[msg->x_coord][msg->y_coord] = msg->letter;
                    scrabbleGameAddress->p1Points = msg->p1Points;
                    scrabbleGameAddress->p2Points = msg->p2Points;


                    /* Delete used tile */
                    for (u = 0; u < 5; u++)
                        if (playerTiles[u] == msg->letter) {
                            playerTiles[u] = 'x';
                            break;
                        }
                    printf("[Client %d] Tiles_current:  %c %c %c %c %c\n", clientDescriptor, playerTiles[0],
                            playerTiles[1], playerTiles[2], playerTiles[3], playerTiles[4]);
                    scrabble_game_print_available_tiles(scrabbleGameAddress->avTiles, 25);

                    fd =fopen(waitingPlayer->fileName, "a");
                    if(bulk_fwrite(fd,gameData)<0) ERR("write:");
                    clearGameDataString(gameData);
                    fclose(fd);

                    if (playerType == SECOND) {
                        semaphore_unlock(gameSemId, 1, 0);
                    } else {
                        semaphore_unlock(gameSemId, 2, 0);
                    }

                } else if (msg->msg == PLAY_ANOTHER_GAME) {
                    if(msg->isMatchOngoing == -1){
                        scrabbleGameAddress->didClientDisconnect = 0;
                        dead = 1;
                        g_doWork = 1;
                        if (playerType == SECOND) {
                            semaphore_unlock(gameSemId, 1, 0);
                        }
                        else {
                            printf("Cleaning up memory by client server:[%d] in Play another game match is ongoing \n",
                                   clientDescriptor);
                            printf("[cleanUpMemoryBool] = %d\n", cleanUpMemoryBool);
                            if (cleanUpMemoryBool) {

                                cleanUp(msg, msg2, scrabbleGameAddress, &scrabbleGameId, gameSemId);
                                cleanUpMemoryBool = 0;
                            }
                        }
                    }
                    else {
                        printf("play another game!!!\n");
                        /***test***/
                        scrabbleGameAddress->didClientDisconnect = 0;
                        dead = 1;
                        g_doWork = 1;
                        printf("[cleanUpMemoryBool] = %d\n", cleanUpMemoryBool);
                        if (cleanUpMemoryBool) {
                            printf("CLEAN UP memory by client server:[%d] in Play another game match is not ongoing\n",
                                   clientDescriptor);

                            cleanUp(msg, msg2, scrabbleGameAddress, &scrabbleGameId, gameSemId);
                            cleanUpMemoryBool = 0;
                        }

                    }

                } else if (msg->msg == FINISH_GAME) {

                    //If match has ended properly
                    printf("is  match ongoing: %d \n", msg->isMatchOngoing);
                    if(msg->isMatchOngoing == -1){
                        printf("Finish the game NIBY JAK TU WCHODZI!!!\n");
                        scrabbleGameAddress->didClientDisconnect = 0;
                        dead = 1;
                        g_doWork = 0;
                        if (playerType == SECOND) {
                            semaphore_unlock(gameSemId, 1, 0);
                        }
                        else {
                            printf("[cleanUpMemoryBool] = %d\n", cleanUpMemoryBool);
                            if (cleanUpMemoryBool) {
                                printf("CLEAN UP memory by client server:[%d] in Finish the game FIRST PLAYER match is not ongoing\n",
                                       clientDescriptor);

                                cleanUp(msg, msg2, scrabbleGameAddress, &scrabbleGameId, gameSemId);
                                cleanUpMemoryBool = 0;
                            }
                        }
                    }
                        //if one of the players left
                    else {
                        printf("Finish the game!!!\n");
                        /***test***/
                        scrabbleGameAddress->didClientDisconnect = 0;
                        dead = 1;
                        g_doWork = 0;

                        printf("[cleanUpMemoryBool] = %d \n", cleanUpMemoryBool);
                        printf("TEST\n");
                        if (cleanUpMemoryBool) {
                            printf("CLEAN UP memory by client server:[%d] in Finish the game, match is not ongoing \n",
                                   clientDescriptor);
                            cleanUp(msg, msg2, scrabbleGameAddress, &scrabbleGameId, gameSemId);
                            cleanUpMemoryBool = 0;
                        }

                    }
                } else
                    printf("[Client %d] Error in packet sent from client.", clientDescriptor);


                printf("unlocked properly!!!\n");
            }

        }
    };

    printf("Closing descriptor \n");
    if(TEMP_FAILURE_RETRY(close(clientDescriptor))<0)ERR("close");

    //printf("isDisconnected = %d, dead = %d\n",isThisClientDisconnected, dead);
    if (isThisClientDisconnected && dead) {
        /* Clean up */

        printf("[CLIENT %d]CLEANUP \n", clientDescriptor);
        msg->msg = EXIT;
       // tcp_socket_send_packet(clientDescriptor, msg);
        printf("[cleanUpMemoryBool] = %d\n", cleanUpMemoryBool);
        if(cleanUpMemoryBool) {
            printf("CLEAN UP memory by client server:[%d] at the very end of server child process \n",
                   clientDescriptor);
            cleanUp(msg, msg2, scrabbleGameAddress, &scrabbleGameId, gameSemId);

        }
    }


}
void clearGameDataString(char* gameDataEntry){
    int i;
    for(i=0;i<GAME_DATA_ENTRY;i++){
        gameDataEntry[i]='\0';
    }
}

void cleanUp(packet* msg,packet* msg2,game* scrabbleGameAddress,int*scrabbleGameId,int gameSemId){
        free(msg);
        free(msg2);
        shared_mem_detach((char*) scrabbleGameAddress);
        shared_mem_delete(*scrabbleGameId);
        semaphore_remove(gameSemId);
}

void sigint_handler(int sig) {
	g_doWork = 0;
    printf("SIGINT\n");
}

void sigterm_handler(int sig)
{
	g_doWork = 0;
}


void sigchld_handler(int sig) {
    g_doWork =0;
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void terminate_children()
{
	printf("TERMINATING CHILDREN!\n");
	int i;
	
	i = 0;
	
	while(children[i] != 0)
	{
		kill(children[i], SIGTERM);
		i++;
	}
}

void save_children_pid(int pid)
{
	int i;	
	i = 0;
	
	while(children[i] != 0)	i++;
	
	children[i] = pid;
	
	//print
	for(i = 0; i < 100; i++) printf("%d ", children[i]);
}
