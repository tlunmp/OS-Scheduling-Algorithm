
#define SHMKEY 9784

#define MAX_USER_PROCESSES 18

typedef struct nodeObject {
	int processId;
	nodeObject* next_node;
} NodeObject;

typedef struct QueueObject {
    NodeObject* front;
    NodeObject* back;
    int timeQuantum;
} QueueObject;


typdef struct message {
	long messageAddress;
	int returnAddress;
} Message;

typedef struct clock {
	int seconds;
	int nanoSeconds;
} Clock;

typedef struct proccessControlBlock{
	Clock processStarts;
	Clock processArrives;
	int turnAroundTime;
	int waitTime;
	int priority;
	int burstTime;
	int timeQuantum;
	int finishedTime;
	int cpuUsageTime;
} ProcessControlBlock;

typedef struct sharedObject {
    ProcessControlBlock processControlBlock [MAX_USER_PROCESSES + 1];
    Clock clockInfo; 
} SharedObject;


