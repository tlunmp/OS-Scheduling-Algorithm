#include "control.h"


#define MESSAGEKEY 3000

typedef struct message {
    long myType;
    char mtext[512];
} Message;


void signalCall(int signum);

int shmid; 
SharedObject* shmPtr;
int messageQueueId;
Clock maxTimeBetweenNewProcs;
Clock userReal;
int times;
int maxforks = 1;
static QueueObject* multilevel_queue_system[4];

QueueObject* create_a_new_queue(int time_quantum_amt) {
    QueueObject* new_queue = (QueueObject*) malloc(sizeof(QueueObject));

    
    if (new_queue == NULL) {
        return NULL;
    }
    new_queue->timeQuantum = time_quantum_amt;
    new_queue->front = NULL;
    new_queue->back = NULL;

    return new_queue;
}


int push_enqueue(QueueObject* destinationQueue, int processID) {

    if (destinationQueue == NULL) {
        return -1;
    }

    NodeObject* new_queue_node = (NodeObject*) malloc(sizeof(NodeObject));
    new_queue_node->processId = processID;
    new_queue_node->nextNode = NULL;


    if (destinationQueue->front == NULL) {
        destinationQueue->front = new_queue_node;
        destinationQueue->back = new_queue_node;
    }
   
    else {
        destinationQueue->back->nextNode = new_queue_node;
        destinationQueue->back = new_queue_node;
    }


    return 0;
}



int pop_dequeue(QueueObject* sourceQueue) {

    //Empty Queue Processing:
        if(sourceQueue->front == NULL){
               return -1; //this is an empty queue
         }
    
        int processID = sourceQueue->front->processId; 
 
         //Remove Node From Queue:
         NodeObject* temporary_node = sourceQueue->front;
         sourceQueue->front = sourceQueue->front->nextNode;
         free(temporary_node);
    
        return processID; 
 }



//signal calls
void signalCall(int signum)
{
    int status;
  //  kill(0, SIGTERM);
    if (signum == SIGINT)
        printf("\nSIGINT received by main\n");
    else
        printf("\nSIGALRM received by main\n");
 
    while(wait(&status) > 0) {
        if (WIFEXITED(status))  /* process exited normally */
                printf("User process exited with value %d\n", WEXITSTATUS(status));
        else if (WIFSIGNALED(status))   /* child exited on a signal */
                printf("User process exited due to signal %d\n", WTERMSIG(status));
        else if (WIFSTOPPED(status))    /* child was stopped */
                printf("User process was stopped by signal %d\n", WIFSTOPPED(status));
    }
    kill(0, SIGTERM);
    //clean up program before exit (via interrupt signal)
    shmdt(shmPtr); //detaches a section of shared memory
    shmctl(shmid, IPC_RMID, NULL);  // deallocate the memory
   
      exit(EXIT_SUCCESS);
 }



//return 0 if realtime or 1 if user process. Probablity is 10%
int userOrRealtime(){
    srand(rand()+ shmPtr->clockInfo.nanoSeconds);
    int chance = rand() % 100;
    if (chance == chance % 10){
                    return 0;
	}
    return 1;
}


void setupPCB(int i, pid_t childpid) {

		shmPtr->processControlBlock[i].pid = childpid;
		shmPtr->processControlBlock[i].priority = 0;
       		shmPtr->processControlBlock[i].cpuUsageTime = 0;
     		shmPtr->processControlBlock[i].burstTime = 0;
       	        shmPtr->processControlBlock[i].timeQuantum = 0;
        	shmPtr->processControlBlock[i].finishedTime = 0;
        	shmPtr->processControlBlock[i].processStarts.seconds = shmPtr->clockInfo.seconds; 
		shmPtr->processControlBlock[i].processStarts.nanoSeconds = shmPtr->clockInfo.nanoSeconds; 
       		shmPtr->processControlBlock[i].processArrives.seconds = 0; 
      		shmPtr->processControlBlock[i].processArrives.nanoSeconds = 0; 
}



void interval() {
	int  seconds = 0;
	int  nanoSeconds = 0;
	times = 0;

	srand(time(NULL));

	printf("maxtime are is %d\n",maxTimeBetweenNewProcs.nanoSeconds);	
	
	times = rand() +  maxTimeBetweenNewProcs.nanoSeconds;

	printf("times generated what  is %d\n",times);

	seconds = maxTimeBetweenNewProcs.seconds;
	nanoSeconds = times;

	while(times > 1000000000){	
			seconds++;
			nanoSeconds -= 1000000000;
	} 
	

	maxTimeBetweenNewProcs.seconds += seconds;
	maxTimeBetweenNewProcs.nanoSeconds += nanoSeconds;
	
}



int main(int argc, char* argv[]) {

	
	
	int bufSize = 200;
	int timer = 20;
	char errorMessage[bufSize];
	Message message;

	pid_t chilpid;
	int ptr_count = 0;

	
	//signal error
	 if (signal(SIGINT, signalCall) == SIG_ERR) {
        	snprintf(errorMessage, sizeof(errorMessage), "%s: Error: user: signal(): SIGINT\n", argv[0]);
		perror(errorMessage);	
        	exit(errno);
  	  }
	
	//sigalarm error
	if (signal(SIGALRM, signalCall) == SIG_ERR) {
            snprintf(errorMessage, sizeof(errorMessage), "%s: Error: user: signal(): SIGALRM\n", argv[0]);
	     perror(errorMessage);	
         	exit(errno);
     	}
	
	//alarm for 2 real life second
	alarm(timer);
	
	if ((shmid = shmget(SHMKEY, sizeof(SharedObject), IPC_CREAT | 0600)) < 0) {
        	perror("Error: shmget");
        	exit(errno);
   	 }
  
 
	if ((messageQueueId = msgget(MESSAGEKEY, IPC_CREAT | 0644)) == -1) {
        	perror("Error: mssget");
       		 exit(errno);
    	}
  
	 shmPtr = shmat(shmid, NULL, 0);
  	 shmPtr->clockInfo.seconds = 0; 
   	 shmPtr->clockInfo.nanoSeconds = 100;  

	pid_t childpid;
	
	int totalCount = 0;
	int maxChildProcess = 3;
	
	maxTimeBetweenNewProcs.seconds = 0;
	maxTimeBetweenNewProcs.nanoSeconds = 0;
	
	int looping = 0;
	int lines = 100;
	
	int i=0;
	
	maxTimeBetweenNewProcs.nanoSeconds = 200000;

	int numChildProcess = 0;

	while(totalCount < maxChildProcess && totalCount < lines ){ 					



			shmPtr->clockInfo.nanoSeconds += 20000;
			//clock incrementation
			if(shmPtr->clockInfo.nanoSeconds > 1000000000){
				shmPtr->clockInfo.seconds++;
				shmPtr->clockInfo.nanoSeconds -= 1000000000;
			}				
		

		
			if(waitpid(0,NULL, WNOHANG)> 0)
				ptr_count--;


			if(shmPtr->clockInfo.seconds == maxTimeBetweenNewProcs.seconds && shmPtr->clockInfo.nanoSeconds > maxTimeBetweenNewProcs.nanoSeconds){	
				char buffer1[100];
				sprintf(buffer1, "%d", totalCount);
				childpid=fork();
				
				shmPtr->processControlBlock[totalCount].pid = childpid;
				shmPtr->processControlBlock[totalCount].priority = userOrRealtime();
     	  			shmPtr->processControlBlock[totalCount].cpuUsageTime = 0;
     				shmPtr->processControlBlock[totalCount].burstTime = 0;
       	       			shmPtr->processControlBlock[totalCount].timeQuantum = 0;
        			shmPtr->processControlBlock[totalCount].finishedTime = 0;
        			shmPtr->processControlBlock[totalCount].processStarts.seconds = shmPtr->clockInfo.seconds; 
				shmPtr->processControlBlock[totalCount].processStarts.nanoSeconds = shmPtr->clockInfo.nanoSeconds; 
       				shmPtr->processControlBlock[totalCount].processArrives.seconds = 0; 
	      			shmPtr->processControlBlock[totalCount].processArrives.nanoSeconds = 0; 


				ptr_count++;
				totalCount++;
		
				
				if(childpid < 0) {
					perror("Fork failed");
				} else if(childpid == 0) {		
					execl("./user", "user", buffer1,(char*)0);
					snprintf(errorMessage, sizeof(errorMessage), "%s: Error: ", argv[0]);
	    	 			perror(errorMessage);		
					exit(0);
				} else {
					
				}


				message.myType = 1;	
				strcpy(message.mtext,"20");
	
				if(msgsnd(messageQueueId, &message,sizeof(message)+1,0) == -1) {
					perror("msgsnd");
					exit(1);
				}

	

				if (msgrcv(messageQueueId, &message,sizeof(message)+1,2,0) == -1) {
					perror("msgrcv");

				}	

				printf("recieve from user is %s\n",message.mtext);

				//printf("process id of each %d\n", shmPtr->processControlBlock[totalCount].pid);
				if(strcmp(message.mtext, "terminated") == 0){
					

				} else {

	     				shmPtr->processControlBlock[totalCount].burstTime = atoi(message.mtext);
					printf("%d", shmPtr->processControlBlock[totalCount].priority);
					
				}

				maxTimeBetweenNewProcs.nanoSeconds += 200000;
		}
	
	}


/*	
	shmdt(shmPtr); //detaches a section of shared memory
    	shmctl(shmid, IPC_RMID, NULL);  // deallocate the memory 
*/
	return 0;
}
