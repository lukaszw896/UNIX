#include <netdb.h>
#include "semaphore_util.h"

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

void semaphore_init(int* semId, char semName, int n)
{
	int i;
	key_t key;
	union semun tmp;
	
	key = ftok("./", semName);
	tmp.val = 1;
	
	/* Create semaphore */
	if((*semId = semget(key, n, 0666 | IPC_CREAT))==-1){
		ERR("semget");
		exit(1);
	}

	/* Set value of each semaphore in the set to 1 */
	for(i = 0; i < n; i++)
	{
		semctl(*semId, i, SETVAL, tmp);
	}
}

void semaphore_remove(int semId)
{
	if(semctl(semId, 0, IPC_RMID) == -1)
	{
		perror("Sem_op error\n");
		exit(1);
	}
}

void semaphore_lock(int semId, short semIndex ,short flag)
{
	struct sembuf tmp;
	
	/* Set operations */
	tmp.sem_num = semIndex;
	tmp.sem_op  = -1;
    tmp.sem_flg = flag;
    
    /* perform value change */
    if(semop(semId, &tmp, 1) == -1)
	{
		perror("Sem_op error\n");
		exit(1);
	}
}

void semaphore_unlock(int semId,short semIndex, short flag)
{
	struct sembuf tmp;
	
	/* Set operations */
	tmp.sem_num = semIndex;
	tmp.sem_op  = 1;
    tmp.sem_flg = flag;
    
    /* perform value change */
    if(semop(semId, &tmp, 1) == -1)
	{
		perror("Sem_op error\n");
		exit(1);
	}
}
