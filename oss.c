#include <stdio.h>
#include "control.h"


#define MESSAGEKEY 3000



typedef struct message {
    long myType;
    char mtext[512];
} Message;


int shmid; 
SharedObject* shmPtr;
int messageQueueId;
Clock maxTimeBetweenNewProcs;



int main(int argc, char* argv[]) {

	int bufSize = 200;

	Message message;

	pid_t chilpid;
	int ptr_count = 0;

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
	int maxChildProcess = 1;
	
	maxTimeBetweenNewProcs.seconds = 0;
	maxTimeBetweenNewProcs.nanoSeconds = 0;
	

	int looping = 0;
	
	int i=0;
	


	//only run the max child
	while(looping < 1){
		
		
		shmPtr->processControlBlock[i].priority = 0;
       		shmPtr->processControlBlock[i].cpuUsageTime = 0;
     		shmPtr->processControlBlock[i].burstTime = 0;
       	        shmPtr->processControlBlock[i].timeQuantum = 0;
        	shmPtr->processControlBlock[i].finishedTime = 0;
        	shmPtr->processControlBlock[i].processStarts.seconds = shmPtr->clockInfo.seconds; 
		shmPtr->process_control_block[i].processStarts.nano_seconds = shmPtr->clockInfo.nanoSeconds; 
       		shmPtr->process_control_block[i].process_arrives.seconds = 0; 
      		shmPtr->process_control_block[i].process_arrives.nano_seconds = 0; 



		shmPtr->clockInfo.nanoSeconds += 20000;
	
		//clock incrementation
		if(shmPtr->clockInfo.nanoSeconds > 1000000000){
			shmPtr->clockInfo.seconds++;
			shmPtr->clockInfo.nanoSeconds -= 1000000000;
		}				
		


			if(waitpid(0, NULL, WNOHANG) > 0){
				ptr_count--;
			}

		//launch time call exec to user.c
		if(shmPtr->clockInfo.seconds == seconds && shmPtr->clockInfo.nanoSeconds > nanoSeconds){		
			
			childpid = fork();
			totalCount++;
			ptr_count++;
					
			if(childpid == 0){
				char buffer2[bufSize];
				sprintf(buffer2, "%d", 20);	
				execl("./user","user",buffer2,(char *)0);

				exit(0);

			} else {
			
			}
	 
 			message.myType = 1;	
			strcpy(message.mtext,"20");
	
			if(msgsnd(messageQueueId, &message,sizeof(message)+1,0) == -1) {
				perror("msgsnd");
				exit(1);
			}
			
			looping++;	
		}	
	}
	return 0;
}



void clock(Clock *destClock, Clock sourceClock, int addNanoSeconds) {
/*
           destClock->nanoSeconds = sourceClock.nanoSeconds + addNanoSeconds;
           if(destClock->nanoSeconds > 1000000000) {
                  destClock->seconds++;
                  destClock->nanoSeconds -= 1000000000;
            }
*/ 
}

/*
void mailMessage(int destAddress){
    static int sizeOfMessage;
    Message message;
    message.messageAddress = destAddress;
    sizeOfMessage = sizeof(message) - sizeof(message.messageAddress);
            msgsnd(messageQueueId, &message, sizeOfMessage, 0);  
 }
*/
