#ifndef HEADERFILE_H
#define HEADERFILE_H


#define MAX_USER_PROCESS 18
#define QUANT1 2000000
#define QUANT2 4000000
#define QUANT3 8000000
#define QUANT4 16000000


typedef struct Node_Object nodeObject;

typedef struct NodeObject{
	int processId;
	nodeObject* nextNode;
} NodeObject;

typedef struct QueueObject {
    NodeObject* front;
    NodeObject* back;
    int timeQuantum;
} QueueObject;

typedef struct SystemClock {
    int seconds; 
    int nanoSeconds;
} SystemClock;

//message queue implementation
typedef struct Message {
    long messageAaddress;
    int returnAddress; 
} messages;


typedef struct ProccessControlBlock{
	SystemClock processStarts;
	SystemClock processArrives;
	int turnAroundTime;
	int waitTime;
	int priority;
	int burst;
	int finishedTime;
	int cpuUsageTime;
	int active;
} ProcessControlBlock;


typedef struct sharedMemoryPCB {
    ProcessControlBlock  processControlBlock[MAX_USER_PROCESS + 1]; 
    SystemClock clockInfo;
} shareMemoryPCB;

void test();
void clock(SystemClock *destClock, SystemClock sourceClock, int addNanoSeconds);
void queue(QueueObject* destObject, int proccesID);
QueueObject* createNewQueue(int time_quantum_amt);


#endif
