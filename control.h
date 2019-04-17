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

#define ADDRESS 900
#define SetBit(A,k)     ( A[((k-1)/32)] |= (1 << ((k-1)%32)) )
#define ClearBit(A,k)   ( A[((k-1)/32)] &= ~(1 << ((k-1)%32)) )
#define TestBit(A,k)    ( A[((k-1)/32)] & (1 << ((k-1)%32)) )
#define TIME_QUANTUM 2000000
#define LINE_MAX 10000
#define MAX_USER_PROCESSES 18
#define CONSTANTNUMBER 500
#define SHM_KEY 3786
#define MESSAGEKEY 3000



typedef struct Node_Object NodeObject;

typedef struct Node_Object {
	int process_id;
	NodeObject* next_node;
} NodeObject;

typedef struct Queue_Object {
    NodeObject* front;
    NodeObject* back;
    int time_quantum;
} QueueObject;

typedef struct system_clock {
    int seconds;
    int nanoSeconds;
} Clock;

//message queue implementation
typedef struct message {
    long messageAddress;
    int returnAddress;
} Message;

typedef struct processCB {
    Clock process_starts;
    Clock process_arrives;
    int turn_around_time;
    int wait_time;
    int priority;
    int burst;
    int time_quantum;
    int finished;
    int cpu_usage_time;
} PCB;

typedef struct shared_memory_object {
    PCB processCB [MAX_USER_PROCESSES + 1];
    Clock clockInfo;
} SharedMemory; 


static QueueObject* multilevelQueue[4];

QueueObject* createNewQueue(int time_quantum_amt);
push_enqueue(QueueObject* destinationQueue, int processID);
