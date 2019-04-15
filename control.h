#pragma once
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/shm.h>
#include <string.h>
#include <math.h>

#define MAX_USER_PROCESSES 18
#define CONTEXT_SWITCH_TIME 500
#define SHM_KEY 9823
#define MESSAGE_QUEUE_KEY 3318
#define MASTER_PROCESS_ADDRESS 999
#define SetBit(A,k)     ( A[((k-1)/32)] |= (1 << ((k-1)%32)) ) 
#define ClearBit(A,k)   ( A[((k-1)/32)] &= ~(1 << ((k-1)%32)) ) 
#define TestBit(A,k)    ( A[((k-1)/32)] & (1 << ((k-1)%32)) )
#define TIME_QUANT_QUEUE_0 2000000
#define TIME_QUANT_QUEUE_1 4000000
#define TIME_QUANT_QUEUE_2 8000000
#define LINE_MAX 10000


typedef struct Node_Object node_object_t;

typedef struct Node_Object {
	int process_id;
	node_object_t* next_node;
} node_object_t;

typedef struct Queue_Object {
    node_object_t* front;
    node_object_t* back;
    int time_quantum;
} queue_object_t;

typedef struct system_clock {
    int seconds;
    int nano_seconds;
} system_clock_t;

//message queue implementation
typedef struct message {
    long message_address;
    int return_address;
} message_t;

typedef struct process_control_block {
    system_clock_t process_starts;
    system_clock_t process_arrives;
    int turn_around_time;
    int wait_time;
    int priority;
    int burst;
    int time_quantum;
    int finished;
    int cpu_usage_time;
} process_control_block_t;

typedef struct shared_memory_object {
    process_control_block_t process_control_block [MAX_USER_PROCESSES + 1];
    system_clock_t clock_info;
} shared_memory_object_t; 


static queue_object_t* multilevel_queue_system[3];

queue_object_t* create_a_new_queue(int time_quantum_amt);
push_enqueue(queue_object_t* destinationQueue, int processID);
