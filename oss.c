#include "control.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>	
#include <semaphore.h>
#include <sys/shm.h>
#include <fcntl.h>
#define SHMKEY 9784

static QueueObject* multilevel[3];

ProcessControlBlock  processControlBlock[18];




int main(int argc, char* argv[]) {
	
	
	test(); 
	return 0;
}


