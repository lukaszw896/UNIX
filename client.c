
/* System includes */
#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

/* Custom includes */
#include "signals_util.h"
#include "scrabble_game.h"
#include "semaphore_util.h"
#include "tcp_socket_util.h"
#include "settings.h"

/*
 * When SIGINT or SIGTERM is catched, this variable will tell main loop to stop.
 */
volatile sig_atomic_t g_doWork = 1;

void gather_input(char*, int*, int*,int*, char[5], char[5][5]);
void otherPlayerDisconnected(packet* msg,int s);
void sigint_handler(int sig);

void sigterm_handler(int sig);

#define HERR(source) (fprintf(stderr,"%s(%d) at %s:%d\n",source,h_errno,__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))
/***********************  TCP part  *************************************************/
int make_socket(void){
	int sock;
	sock = socket(PF_INET,SOCK_STREAM,0);
	if(sock < 0) ERR("socket");
	return sock;
}

struct sockaddr_in make_address(char *address, uint16_t port){
	struct sockaddr_in addr;
	struct hostent *hostinfo;
	addr.sin_family = AF_INET;
	addr.sin_port = htons (port);
	hostinfo = gethostbyname(address);
	if(hostinfo == NULL)HERR("gethostbyname");
	addr.sin_addr = *(struct in_addr*) hostinfo->h_addr;
	return addr;
}

int connect_socket(char *name, uint16_t port){
	struct sockaddr_in addr;
	int socketfd;
	socketfd = make_socket();
	addr=make_address(name,port);
	if(connect(socketfd,(struct sockaddr*) &addr,sizeof(struct sockaddr_in)) < 0){
		if(errno!=EINTR) ERR("connect");
		else { 
			fd_set wfds ;
			int status;
			socklen_t size = sizeof(int);
			FD_ZERO(&wfds);
			FD_SET(socketfd, &wfds);
			if(TEMP_FAILURE_RETRY(select(socketfd+1,NULL,&wfds,NULL,NULL))<0) ERR("select");
			if(getsockopt(socketfd,SOL_SOCKET,SO_ERROR,&status,&size)<0) ERR("getsockopt");
			if(0!=status) ERR("connect");
		}
	}
	return socketfd;
}

/**********************************************************************************/


int main(void) {
    char decission[50];
    int s, sem;
    packet* rec = malloc(sizeof (packet));
    packet* tmp = malloc(sizeof (packet));
    char c;
    int x, y, points;
    semaphore_init(&sem, 'E', 1);
    
    /* Set signals handlers */
	if(sethandler(sigint_handler,SIGINT)) ERR("Setting SIGINT:");
	//if(sethandler(sigchld_handler,SIGCHLD)) ERR("Setting SIGCHLD:");
	if(sethandler(sigterm_handler,SIGTERM)) ERR("Setting SIGTERM:");

    /*********  TCP IP  *********/

    s = connect_socket("localhost", 2000);
    /****************************/
    /*
    socket_init_unix(&s,&remote, SOCK_PATH);   
    socket_connect(&s,&remote);
     * */
    printf("############################\n");
    printf("# WELCOME TO SCRABBLE GAME #\n");
    printf("############################\n");


    while (g_doWork) {
        int t;

        if ((t = tcp_socket_read_packet(s, rec)) == 0 && g_doWork==1) {
            perror("Server closed connection client\n");
            return t;
        } else if (t < 0) {
            perror("recv");
            return t;
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

                points = 0;
                gather_input(&c, &x, &y, &points, rec->tiles, rec->currentBoard);

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
                break;
            case DISCONNECTED:

                printf("########################################\n");
                printf("#            Connection lost!          #\n");
                printf("########################################\n");
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
                break;
                }
                break;
                
            case EXIT:
                printf("Server disconnected.\n");
                return EXIT_SUCCESS;
                break;
            default:
                printf("Cannot interpret packet's message: %d \n", rec->msg);
        }
    };
    printf("Disconnected\n");
    close(s);
    return 0;
}

/*void otherPlayerDisconnected(packet* msg,int s){
        char* c;
      printf("########################################\n");
      printf("#            Connection lost!          #\n");
      printf("########################################\n");
      printf("Sorry but connection with other player is\n");
      printf("lost. Do you want to play another game?(Y/N)\n");
      scanf(" %c", c);
      if(*c == 'Y'){
          msg->msg = PLAY_ANOTHER_GAME;
          
      }
      else if(*c == 'N'){
         msg->msg = FINISH_GAME; 
      }
      tcp_socket_send_packet(s, msg);
}*/

void gather_input(char* c, int* x, int* y, int* points, char tiles[5], char board[5][5])
{
	int i, k, j;
	
	printf("Supply <letter> <x> <y>\n");
	while(1)
	{
		scanf(" %c %d %d", c, x, y);
		
		/* Basic border check */
		if(*x < 0 || *y < 0 || *x > 4 || *y > 4)
		{
			printf("Please... x and y must be from <0;4> interval.\n");
			continue;
		}
		
		/* Check if given character is on the list */
		k = 1;
		for(i = 0; i < 5; i++)
			if(tiles[i] == *c)
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
				int n_x = *x + i;
				int n_y = *y + j;
				
				if(n_x >= 0 && n_x < 5 && n_y >= 0 && n_y < 5)
				{
					if(board[n_x][n_y] != 'x')
						k = 0;
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
}

void sigint_handler(int sig) {
	g_doWork = 0;
        printf("sigint handler");
}

void sigterm_handler(int sig)
{
	g_doWork = 0;
}
