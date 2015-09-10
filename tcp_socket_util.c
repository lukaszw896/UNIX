#include "tcp_socket_util.h"

void tcp_socket_init_unix(int *descriptor, struct sockaddr_in* soc, char* name)
{
	if ((*descriptor = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    soc->sin_family = AF_INET;
    //strcpy(soc->sun_path, name);
    soc->sin_port = htons(2000);
	soc->sin_addr.s_addr = htonl(INADDR_ANY);

}

void tcp_socket_bind(int* descriptor, struct sockaddr_in* soc)
{
	//int len;
	int t=1;

    //len = strlen(soc->sun_path) + sizeof(soc->sun_family);
    
	//unlink(soc->sun_path);
	
	if (setsockopt(*descriptor, SOL_SOCKET, SO_REUSEADDR,&t, sizeof(t))) ERR("setsockopt");
	if(bind(*descriptor,(const struct sockaddr*) soc,sizeof(*soc)) < 0) {
		 ERR("bind");
		 exit(1);
	 }
	
    /*if (bind(*descriptor, (struct sockaddr *)soc, len) == -1) {
        perror("bind");
        exit(1);
    }*/
}

void tcp_socket_connect(int* descriptor, struct sockaddr_in* soc)
{
	//int len;
		
	//len = strlen(soc->sun_path) + sizeof(soc->sin_family);
	
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
	//printf("In send packet: %d \n",pac->isMatchOngoing);
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
		
		if ((t = recv(socket, serialized, sizeof(serialized), 0))  == 0) 
		{
			perror("Server closed connection\n");
			return t;
        } else if(t < 0){
			perror("recv");
            return t;
        }

        tcp_socket_deserialize(pac, serialized);
	printf("Right after deserialization: %d\n",pac->isMatchOngoing);
        return t;
}


///// HTON
// HARDCODED FOR NOW
void tcp_socket_serialize(packet pac, char* str)
{
	//printf("Right before serilizing = %d\n",pac.isMatchOngoing);
	sprintf(str, 
	"%d%c%c%c%d%c%d%c%d%c%d%c%d%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%d",
	
	pac.msg, SEPARATOR, pac.letter, SEPARATOR, pac.x_coord, SEPARATOR, pac.y_coord, 
	SEPARATOR, pac.p1Points, SEPARATOR, pac.p2Points, SEPARATOR, pac.playerType,
	
	SEPARATOR, pac.currentBoard[0][0], SEPARATOR, pac.currentBoard[0][1], SEPARATOR, pac.currentBoard[0][2], SEPARATOR, pac.currentBoard[0][3], SEPARATOR, pac.currentBoard[0][4], 
	SEPARATOR, pac.currentBoard[1][0], SEPARATOR, pac.currentBoard[1][1], SEPARATOR, pac.currentBoard[1][2], SEPARATOR, pac.currentBoard[1][3], SEPARATOR, pac.currentBoard[1][4], 
	SEPARATOR, pac.currentBoard[2][0], SEPARATOR, pac.currentBoard[2][1], SEPARATOR, pac.currentBoard[2][2], SEPARATOR, pac.currentBoard[2][3], SEPARATOR, pac.currentBoard[2][4], 
	SEPARATOR, pac.currentBoard[3][0], SEPARATOR, pac.currentBoard[3][1], SEPARATOR, pac.currentBoard[3][2], SEPARATOR, pac.currentBoard[3][3], SEPARATOR, pac.currentBoard[3][4], 
	SEPARATOR, pac.currentBoard[4][0], SEPARATOR, pac.currentBoard[4][1], SEPARATOR, pac.currentBoard[4][2], SEPARATOR, pac.currentBoard[4][3], SEPARATOR, pac.currentBoard[4][4],
	
	SEPARATOR, pac.tiles[0], SEPARATOR, pac.tiles[1], SEPARATOR, pac.tiles[2], SEPARATOR, pac.tiles[3], SEPARATOR, pac.tiles[4]	,SEPARATOR, pac.isMatchOngoing

	);
	//printf("Serialize:%s\n",str);
} 
// HARDCODED FOR NOW
void tcp_socket_deserialize(packet* pac, char* str)
{	char tmp;
	//printf("Before deser:%s\n",str);
	sscanf(str, 
	"%d%c%c%c%d%c%d%c%d%c%d%c%d%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%d",
	
	&(pac->msg), &tmp, &(pac->letter), &tmp, &(pac->x_coord), &tmp, &(pac->y_coord),
		&tmp, &(pac->p1Points), &tmp, &(pac->p2Points), &tmp, &(pac->playerType),
		 
	&tmp, &(pac->currentBoard[0][0]), &tmp, &(pac->currentBoard[0][1]), &tmp, &(pac->currentBoard[0][2]), &tmp, &(pac->currentBoard[0][3]), &tmp, &(pac->currentBoard[0][4]), 
	&tmp, &(pac->currentBoard[1][0]), &tmp, &(pac->currentBoard[1][1]), &tmp, &(pac->currentBoard[1][2]), &tmp, &(pac->currentBoard[1][3]), &tmp, &(pac->currentBoard[1][4]), 
	&tmp, &(pac->currentBoard[2][0]), &tmp, &(pac->currentBoard[2][1]), &tmp, &(pac->currentBoard[2][2]), &tmp, &(pac->currentBoard[2][3]), &tmp, &(pac->currentBoard[2][4]), 
	&tmp, &(pac->currentBoard[3][0]), &tmp, &(pac->currentBoard[3][1]), &tmp, &(pac->currentBoard[3][2]), &tmp, &(pac->currentBoard[3][3]), &tmp, &(pac->currentBoard[3][4]), 
	&tmp, &(pac->currentBoard[4][0]), &tmp, &(pac->currentBoard[4][1]), &tmp, &(pac->currentBoard[4][2]), &tmp, &(pac->currentBoard[4][3]), &tmp, &(pac->currentBoard[4][4]),
	
	&tmp, &(pac->tiles[0]), &tmp, &(pac->tiles[1]), &tmp, &(pac->tiles[2]), &tmp, &(pac->tiles[3]), &tmp, &(pac->tiles[4]),&tmp,&(pac->isMatchOngoing)
	

	 );

	//printf("After deser:%s\n",str);
	// TODO separator check
} 
