#ifndef SEMAPHORE_UTIL_H
#define SEMPAHORE_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>

/*
 * Union needed for semctl() function calls.
 */
union semun {
 int val;
 struct semid_ds *buf;
 unsigned short int *array;
 };

/*
 * Creates a set of semaphore and saves its id.
 * In addition all values are set to 1.
 */
void semaphore_init(int*, char, int);

/*
 *  Destroys semaphore of a given id.
 */
void semaphore_remove(int);

/*
 * Lock semaphore until semaphore_unclock() call.
 */
void semaphore_lock(int,short, short);

/*
 * Unlocks semaphore.
 */
void semaphore_unlock(int,short, short); 

#endif
