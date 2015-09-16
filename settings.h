#ifndef SETTINGS_H
#define SETTINGS_H

#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>


#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))



#define SOCK_PATH 			"scrabble_socket"
#define SEPARATOR			';'
#define UNAVAILABLE			'!'
#define MAX_CHILDREN		 100
#define INCOMMING_CONN 		 5
#define BOARD_WIDTH			 5
#define BOARD_HEIGHT		 5
#define NO_PLAYER			-1
#define PAIR_MATCH			-2
#define NONE				-3
#define FIRST				-4
#define SECOND				-5
#define DISCONNECTED		-6
#define CONNECTED			-7
#define REQUEST_MOVE		-8
#define MOVE_DATA			-9
#define INFO				-10
#define EXIT				-11
#define PLAY_ANOTHER_GAME   -12
#define FINISH_GAME         -13

#endif
