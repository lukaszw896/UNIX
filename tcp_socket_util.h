#ifndef TCP_SOCKET_UTIL_H
#define TCP_SOCKET_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "settings.h"
#include <netinet/in.h>
/*
 * Structure encapsulates data sent over socket.
 */
typedef struct
{
	int msg;
	char letter;
	int x_coord;
	int y_coord;
	int p1Points;
	int p2Points;
	int playerType;
	char currentBoard[BOARD_HEIGHT][BOARD_WIDTH];
	char tiles[5];
} packet;

/*
 * Creates basic socket of AF_UNIX family. 
 * Socket is indetified with its descriptor and special structure.
 */
void tcp_socket_init_unix(int *descriptor, struct sockaddr_in* soc, char* name);

/*
 * Binds given descriptor and socket structure.
 */
void tcp_socket_bind(int*, struct sockaddr_in*);

/*
 * Connects to a provided socket
 * Socket is indetify with its descriptor and special structure.
 */
void tcp_socket_connect(int*, struct sockaddr_in*);

/*
 * Forces socket to start listening for incoming connections.
 * Number of queued connections (before accept()) must be provided.
 */
void tcp_socket_listen(int*, int);

/*
 * Sends provided packet over a socket.
 */
void tcp_socket_send_packet(int, packet*); 

/*
 * Reads packet from a socket.
 */
int tcp_socket_read_packet(int, packet*);

/*
 * Serialize packet;
 */
void tcp_socket_serialize(packet pac, char* str);

/*
 * Deserialize packet;
 */
void tcp_socket_deserialize(packet* pac, char* str);

#endif
