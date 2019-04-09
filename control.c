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
#include "control.h"
#include <stdio.h>


int queue(queueObject* destObject, int proccessID){
    if (destObject == NULL) {
        return -1; //invalid queue
    }

   nodeObject* newQueueNode = (nodeOBject*) malloc(sizeof(nodeObject));

   newQueueNode->processId = processId;
   newQueueNode->nextNode = NULL;

  if(destObject->front == NULL) {
	destObject->front = newQueueNode;
	destObject->back = newQueueNode;
  } else {

	destinationQueue->back->next_node = new_queue_node;
        destinationQueue->back = new_queue_node;
  }

   return 0;

}

void test(){
	printf("hello");
}
void clock(SystemClock *destClock, SystemClock sourceClock, int addNanoSeconds) {
           destinationClock->nano_seconds = sourceClock.nano_seconds + addNanoSeconds;
           if(destinationClock->nano_seconds > 1000000000) {
                  destinationClock->seconds++;
                  destinationClock->nano_seconds -= 1000000000;
            }
 }

QueueObject* createNewQueue(int time_quantum_amt) {
    QueueObject* new_queue = (*) malloc(sizeof(QueueObject));
    	
	//malloc failed 
        if (new_queue == NULL) {
                return NULL;
           }

         new_queue->time_quantum = time_quantum_amt;
         new_queue->front = NULL;
         new_queue->back = NULL;
    
        return new_queue;
   }
   
