#include "shared_mem_util.h"

void shared_mem_init(int* shmId, int size)
{
	if((*shmId=shmget(IPC_PRIVATE, size, IPC_CREAT | 0777)) == -1 )
	{
		printf("semget error!init\n");
		exit(1);
	}
}

char* shared_mem_attach(int shmId)
{
	char* pShm = NULL;
	if((pShm=shmat(shmId, NULL,0))==(char *)-1)
	{
		perror("attach");
		printf("shmat error!attach\n");
		exit(1);
	}
	return pShm;
}

void shared_mem_detach(char* shmAddress)
{
    if (shmdt(shmAddress) == -1) {
        perror("shmdt");
        exit(1);
    }
}

void shared_mem_delete(int shmId)
{
	shmctl(shmId, IPC_RMID, NULL);
}
