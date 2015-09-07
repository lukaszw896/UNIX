#ifndef SIGNALS_UTIL_H
#define SIGNALS_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

/*
 * Used to attach a handler function to the signal.
 */
int sethandler( void (*f)(int), int);

#endif
