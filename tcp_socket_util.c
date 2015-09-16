#define _GNU_SOURCE

#define HERR(source) (fprintf(stderr,"%s(%d) at %s:%d\n",source,h_errno,__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

#include <netdb.h>
#include "tcp_socket_util.h"

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

int tcp_wait_for_client(int *clientSocket, int serverSocket, struct sockaddr_in *remote)
{
	socklen_t sockLength = sizeof(remote);
	if ((*clientSocket = accept(serverSocket, (struct sockaddr *)remote, &sockLength)) == -1)
	{
		if(errno == EINTR || errno == SIGTERM) {
			return -1;
		}
	}
	return 0;
}

void tcp_socket_init_unix(int *descriptor, struct sockaddr_in* soc, char* name)
{
	if ((*descriptor = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    soc->sin_family = AF_INET;
    soc->sin_port = htons(2000);
	soc->sin_addr.s_addr = htonl(INADDR_ANY);

}

void tcp_socket_bind(int* descriptor, struct sockaddr_in* soc)
{
	int t=1;

	
	if (setsockopt(*descriptor, SOL_SOCKET, SO_REUSEADDR,&t, sizeof(t))) ERR("setsockopt");
	if(bind(*descriptor,(const struct sockaddr*) soc,sizeof(*soc)) < 0) {
		 ERR("bind");
		 exit(1);
	 }

}

void tcp_socket_connect(int* descriptor, struct sockaddr_in* soc)
{
	printf("Trying to connect...\n");
    if (connect(*descriptor, (struct sockaddr *)soc, sizeof(*soc)) == -1) {
        perror("connect");
        exit(1);
    }
    printf("Connected.\n");
}
	
void tcp_socket_listen(int* descriptor, int incomingConn)
{
    if (listen(*descriptor, incomingConn) == -1) {
        perror("listen");
        exit(1);
    }
}

void tcp_socket_send_packet(int socket, packet* pac)
{
	char serialized[200];
	tcp_socket_serialize(*pac, serialized);

	if(send(socket, serialized, sizeof(serialized),0) == -1)
	{
		perror("send_packet");
		exit(1);
	}
}

int tcp_socket_read_packet(int socket, packet* pac)
{
		int t;
		char serialized[200];
		
		if ((t =  recv(socket, serialized, sizeof(serialized), 0))  == 0)
		{
			perror("Server closed connection\n");
			return t;
        } else if(t < 0){
			perror("recv");
            return t;
        }

        tcp_socket_deserialize(pac, serialized);
        return t;
}

void tcp_socket_serialize(packet pac, char* str)
{
	sprintf(str,
			"%d%c%c%c%d%c%d%c%d%c%d%c%d%c%d%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",

			pac.msg, SEPARATOR, pac.letter, SEPARATOR, pac.x_coord, SEPARATOR, pac.y_coord,
			SEPARATOR, pac.p1Points, SEPARATOR, pac.p2Points, SEPARATOR, pac.playerType,SEPARATOR, pac.isMatchOngoing,

			SEPARATOR, pac.currentBoard[0][0], SEPARATOR, pac.currentBoard[0][1], SEPARATOR, pac.currentBoard[0][2], SEPARATOR, pac.currentBoard[0][3], SEPARATOR, pac.currentBoard[0][4],
			SEPARATOR, pac.currentBoard[1][0], SEPARATOR, pac.currentBoard[1][1], SEPARATOR, pac.currentBoard[1][2], SEPARATOR, pac.currentBoard[1][3], SEPARATOR, pac.currentBoard[1][4],
			SEPARATOR, pac.currentBoard[2][0], SEPARATOR, pac.currentBoard[2][1], SEPARATOR, pac.currentBoard[2][2], SEPARATOR, pac.currentBoard[2][3], SEPARATOR, pac.currentBoard[2][4],
			SEPARATOR, pac.currentBoard[3][0], SEPARATOR, pac.currentBoard[3][1], SEPARATOR, pac.currentBoard[3][2], SEPARATOR, pac.currentBoard[3][3], SEPARATOR, pac.currentBoard[3][4],
			SEPARATOR, pac.currentBoard[4][0], SEPARATOR, pac.currentBoard[4][1], SEPARATOR, pac.currentBoard[4][2], SEPARATOR, pac.currentBoard[4][3], SEPARATOR, pac.currentBoard[4][4],

			SEPARATOR, pac.tiles[0], SEPARATOR, pac.tiles[1], SEPARATOR, pac.tiles[2], SEPARATOR, pac.tiles[3], SEPARATOR, pac.tiles[4]

	);
}

void tcp_socket_deserialize(packet* pac, char* str)
{	char tmp;
	sscanf(str,
		   "%d%c%c%c%d%c%d%c%d%c%d%c%d%c%d%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",

		   &(pac->msg), &tmp, &(pac->letter), &tmp, &(pac->x_coord), &tmp, &(pac->y_coord),
		   &tmp, &(pac->p1Points), &tmp, &(pac->p2Points), &tmp, &(pac->playerType),&tmp,&(pac->isMatchOngoing),

		   &tmp, &(pac->currentBoard[0][0]), &tmp, &(pac->currentBoard[0][1]), &tmp, &(pac->currentBoard[0][2]), &tmp, &(pac->currentBoard[0][3]), &tmp, &(pac->currentBoard[0][4]),
		   &tmp, &(pac->currentBoard[1][0]), &tmp, &(pac->currentBoard[1][1]), &tmp, &(pac->currentBoard[1][2]), &tmp, &(pac->currentBoard[1][3]), &tmp, &(pac->currentBoard[1][4]),
		   &tmp, &(pac->currentBoard[2][0]), &tmp, &(pac->currentBoard[2][1]), &tmp, &(pac->currentBoard[2][2]), &tmp, &(pac->currentBoard[2][3]), &tmp, &(pac->currentBoard[2][4]),
		   &tmp, &(pac->currentBoard[3][0]), &tmp, &(pac->currentBoard[3][1]), &tmp, &(pac->currentBoard[3][2]), &tmp, &(pac->currentBoard[3][3]), &tmp, &(pac->currentBoard[3][4]),
		   &tmp, &(pac->currentBoard[4][0]), &tmp, &(pac->currentBoard[4][1]), &tmp, &(pac->currentBoard[4][2]), &tmp, &(pac->currentBoard[4][3]), &tmp, &(pac->currentBoard[4][4]),

		   &tmp, &(pac->tiles[0]), &tmp, &(pac->tiles[1]), &tmp, &(pac->tiles[2]), &tmp, &(pac->tiles[3]), &tmp, &(pac->tiles[4])


	);
}

