#include "control.h"
#include <math.h>



QueueObject* createNewQueue(int time_quantum_amt) {
    QueueObject* new_queue = (QueueObject*) malloc(sizeof(QueueObject));

    
    if (new_queue == NULL) {
        return NULL;
    }
    new_queue->time_quantum = time_quantum_amt;
    new_queue->front = NULL;
    new_queue->back = NULL;

    return new_queue;
}

int pushEnqueue(QueueObject* destinationQueue, int processID) {

    if (destinationQueue == NULL) {
        return -1;
    }

    NodeObject* new_queue_node = (NodeObject*) malloc(sizeof(NodeObject));
    new_queue_node->process_id = processID;
    new_queue_node->next_node = NULL;


    if (destinationQueue->front == NULL) {
        destinationQueue->front = new_queue_node;
        destinationQueue->back = new_queue_node;
    }
   
    else {
        destinationQueue->back->next_node = new_queue_node;
        destinationQueue->back = new_queue_node;
    }


    return 0;
}

int pop_dequeue(QueueObject* sourceQueue) {

    //Empty Queue Processing:
        if(sourceQueue->front == NULL){
               return -1; //this is an empty queue
         }
    
        int processID = sourceQueue->front->process_id;
 
         //Remove Node From Queue:
                                    NodeObject* temporary_node = sourceQueue->front;
                                       sourceQueue->front = sourceQueue->front->next_node;
                                           free(temporary_node);
    
                                                return processID; 
 }

void destroyQueue(QueueObject* sourceQueue) {

    NodeObject* current_node = sourceQueue->front;
    NodeObject* temporary_node;

    //Free Each Of The Nodes In The Queue:
    while (current_node != NULL) {
        temporary_node = current_node->next_node;
        free(current_node);
        current_node = temporary_node;
    
    }
    
                                        //Free The Queue Itself:
    free(sourceQueue);
}
