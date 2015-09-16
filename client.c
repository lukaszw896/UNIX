
/* System includes */
#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>


#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

/* Custom includes */
#include "signals_util.h"
#include "scrabble_game.h"
#include "semaphore_util.h"
#include "tcp_socket_util.h"
#include "settings.h"

volatile sig_atomic_t g_doWork = 1;

int gather_input(char*, int*, int*,int*, char[5], char[5][5]);

void sigint_handler(int sig);
void sigterm_handler(int sig);

int main(void) {
    char decission[50];
    int s, sem;
    packet* rec = malloc(sizeof (packet));
    if(rec == NULL){
        ERR("malloc");
    }
    packet* tmp = malloc(sizeof (packet));
    if(tmp == NULL){
        ERR("malloc");
    }
    char c;
    int x, y, points,i;
    int isAnyTileAvaliable;
    semaphore_init(&sem, 'E', 1);
    
    /* Set signals handlers */
	if(sethandler(sigint_handler,SIGINT)) ERR("Setting SIGINT:");
	if(sethandler(sigterm_handler,SIGTERM)) ERR("Setting SIGTERM:");

    /*********  TCP IP  *********/
    s = tcp_connect_socket("localhost", 2000);

    printf("##############################\n");
    printf("## WELCOME TO SCRABBLE GAME ##\n");
    printf("##############################\n");


    while (g_doWork) {
        int t;

        if ((t = tcp_socket_read_packet(s, rec)) == 0 && g_doWork==1) {
            perror("Server closed connection client\n");
            break;
        } else if (t < 0) {
            perror("recv");
            break;
        }
        switch (rec->msg) {
            case NO_PLAYER:
                printf("Waiting for another player to connect...\n");
                break;
            case PAIR_MATCH:
                printf("Player found - starting the game...\n");
                break;
            case INFO:
                printf("Player found - starting the game...\n");
                scrabble_game_print_title();
                scrabble_game_print_board(rec->currentBoard);
                scrabble_game_print_wait_for_move();
                break;
            case REQUEST_MOVE:
                /* Print points in the right order */
                scrabble_game_print_title();
                scrabble_game_print_points(rec->p1Points, rec->p2Points, rec->playerType);

                /* Print current board state */
                scrabble_game_print_board(rec->currentBoard);

                /* Print available tiles */
                printf("----------------------------------------\n              ");
                scrabble_game_print_available_tiles(rec->tiles, 5);
                printf("----------------------------------------\n");
                isAnyTileAvaliable = 0;
                for(i=0; i<5;i++){
                    if(rec->tiles[i] != 'x'){
                        isAnyTileAvaliable = 1;
                    }
                }
                if(isAnyTileAvaliable == 1) {
                    points = 0;
                    if(-1 == gather_input(&c, &x, &y, &points, rec->tiles, rec->currentBoard)) {
                        printf("gater input error\n");
                    }else {
                        /* Print updated points */
                        scrabble_game_print_title();
                        if (rec->playerType == FIRST) rec->p1Points += points;
                        if (rec->playerType == SECOND) rec->p2Points += points;
                        scrabble_game_print_points(rec->p1Points, rec->p2Points, rec->playerType);

                        /* Print updated board */
                        rec->currentBoard[x][y] = c;
                        scrabble_game_print_board(rec->currentBoard);

                        /* Send data to server. */
                        tmp->msg = MOVE_DATA;
                        tmp->letter = c;
                        tmp->x_coord = x;
                        tmp->y_coord = y;
                        tmp->p1Points = rec->p1Points;
                        tmp->p2Points = rec->p2Points;
                        tcp_socket_send_packet(s, tmp);
                        scrabble_game_print_wait_for_move();
                    }
                }
                else{
                    printf("##########################################\n");
                    printf("##             END OF MATCH             ##\n");
                    printf("##########################################\n");
                    printf("Do you want to play another game?(Y/N)\n");
                    scanf(" %c", decission);
                    printf("%s \n",decission);
                    tmp->isMatchOngoing = -1;
                    if (decission[0] == 'Y') {
                        tmp->msg = PLAY_ANOTHER_GAME;
                        tcp_socket_send_packet(s, tmp);
                    }
                    else
                    {
                        tmp->msg = FINISH_GAME;
                        printf("isMatchOngoing = %d \n", tmp->isMatchOngoing);
                        tcp_socket_send_packet(s, tmp);
                        return EXIT_SUCCESS;
                    }
                }
                break;
            case DISCONNECTED:

                printf("##########################################\n");
                printf("##            Connection lost!          ##\n");
                printf("##########################################\n");
                printf("Sorry but connection with other player is\n");
                printf("lost. Do you want to play another game?(Y/N)\n");
                scanf(" %c", decission);
                printf("%s \n",decission);
                if (decission[0] == 'Y') {
                    tmp->msg = PLAY_ANOTHER_GAME;
                    tcp_socket_send_packet(s, tmp);
                } 
                else 
                {
                    tmp->msg = FINISH_GAME;
                    tcp_socket_send_packet(s, tmp);
                    return EXIT_SUCCESS;
                }
                break;
                
            case EXIT:
                printf("Server disconnected.\n");
                return EXIT_SUCCESS;
            default:
                printf("Cannot interpret packet's message: %d \n", rec->msg);
        }
    };
    printf("Disconnected\n");
    if(-1 == close(s)){
        ERR("close");
    }
    return 0;
}

int gather_input(char* c, int* x, int* y, int* points, char tiles[5], char board[5][5])
{
	int i, k, j;
	
	printf("Supply <letter> <y> <x>\n");
	while(1)
	{
		if( 0>scanf(" %c %d %d", c, x, y) ){
            if(errno == EINTR){
                printf("Input error eintr\n");
                return -1;
            }
            else{
               printf("Input error\n");
            }
        }

		/* Basic border check */
		if(*x < 0 || *y < 0 || *x > 4 || *y > 4)
		{
			printf("Please... x and y must be from <0;4> interval.\n");
			continue;
		}
		
		/* Check if given character is on the list */
		k = 1;
		for(i = 0; i < 5; i++)
			if(tiles[i] == *c && *c!='x')
				k = 0;
		if(k == 1)
		{
			printf("Please... letter is not among available ones.\n");
			continue;
		}
		
		/* Check if it is the first move (clean board). */
		k = 1;
		for(i = 0; i < 5; i++)
			for(j = 0; j < 5; j++)	
			{
					if(board[i][j] != 'x')
						k = 0;
			}
		if(k == 1)
		{
			break;
		}
		
		/* Check if movement is possible */
		k = 1;
		for(i = -1; i < 2; i++)
			for(j = -1; j < 2; j++)
			{
                if(!((i==-1 && j==-1) || (i==1 && j==1) || (i==1 && j==-1) || (i==-1 && j==1)) ) {
                    int n_x = *x + i;
                    int n_y = *y + j;

                    if (n_x >= 0 && n_x < 5 && n_y >= 0 && n_y < 5) {
                        if (board[n_x][n_y] != 'x')
                            k = 0;
                    }
                }
			}
		/* Check if tile overlaps other one. */	
		if(board[*x][*y] != 'x')
			k = 1;
		if(k == 1)
		{
			printf("Please... movement is not allowed.\n");
			continue;
		}

		/* At this point everything (for sure) is ok. */
		break;
	}
	*points = scrabble_game_calculate_points(board, *x, *y);

    return 1;
}

void sigint_handler(int sig) {
	g_doWork = 0;
}

void sigterm_handler(int sig)
{
	g_doWork = 0;
}
