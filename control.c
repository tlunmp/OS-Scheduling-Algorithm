#include "control.h"
#include <math.h>



queue_object_t* create_a_new_queue(int time_quantum_amt) {
    queue_object_t* new_queue = (queue_object_t*) malloc(sizeof(queue_object_t));

    
    if (new_queue == NULL) {
        return NULL;
    }
    new_queue->time_quantum = time_quantum_amt;
    new_queue->front = NULL;
    new_queue->back = NULL;

    return new_queue;
}

int push_enqueue(queue_object_t* destinationQueue, int processID) {

    if (destinationQueue == NULL) {
        return -1;
    }

    node_object_t* new_queue_node = (node_object_t*) malloc(sizeof(node_object_t));
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

int pop_dequeue(queue_object_t* sourceQueue) {

    //Empty Queue Processing:
        if(sourceQueue->front == NULL){
               return -1; //this is an empty queue
         }
    
        int processID = sourceQueue->front->process_id; 
 
         //Remove Node From Queue:
                                    node_object_t* temporary_node = sourceQueue->front;
                                       sourceQueue->front = sourceQueue->front->next_node;
                                           free(temporary_node);
    
                                                return processID; 
 }

void destroyQueue(queue_object_t* sourceQueue) {

    node_object_t* current_node = sourceQueue->front;
    node_object_t* temporary_node;

    //Free Each Of The Nodes In The Queue:
        while (current_node != NULL) {
               temporary_node = current_node->next_node;
                       free(current_node);
                                current_node = temporary_node;
        }
    
                                        //Free The Queue Itself:
                                            free(sourceQueue);
                                            }    
