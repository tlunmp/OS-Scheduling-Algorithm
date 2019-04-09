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


typedef struct Node_Object {
	int process_id;
	node_object_t* next_node;
} object_t; //this is a node for a linked-list implementation of a queue

typedef struct Queue_Object {
    node_object_t* front;
    node_object_t* back;
    int time_quantum;
} object_t;


int main(int argc, char* argv[]) {

	int maxChildProcess = 20;

	

	while(totalCount < maxChildProcess && totalCount < lines ){ 					
				
			if(waitpid(0,NULL, WNOHANG)> 0)
				ptr_count--;

			if(ptr_count < 20 && indexOfTheString < lines){
				ptr_count++;
				totalCount++;
		
				childpid=fork();

				if(childpid < 0) {
					perror("Fork failed");
				} else if(childpid == 0) {
				 
					//exec the index and lines tot he child
					//execl("./palin","palin",buffer1,buffer2,(char *)0);
					snprintf(errorMessage, sizeof(errorMessage), "%s: Error: ", arg0Name);
	    	 			perror(errorMessage);		
					exit(0);
				} else {
					
				}
			
				indexOfTheString += 5;
			}
	}

	return 0;
}


