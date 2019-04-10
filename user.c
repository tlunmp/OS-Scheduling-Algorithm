#include "control.h"
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
#include <sys/msg.h>
#include <sys/shm.h>
#include <fcntl.h>
#define SHMKEY 9784
#define MESSAGEKEY 3000

int shmid; 
int child_id;

SharedObject* shmPtr;


typedef struct message {
    long myType;
    char mtext[512];
} Message;

static int messageQueueId;
/*
void mail_the_message(int destination_address);
void receive_the_message(int destination_address);
*/

int main(int argc, char* argv[]) {

	Message message;
	int some = atoi(argv[1]);
//     child_id = atoi(argv[1]);

     if ((shmid = shmget(SHMKEY, sizeof(SharedObject), 0600)) < 0) {
            perror("Error: shmget");
            exit(errno);
     }
   
     if ((messageQueueId = msgget(MESSAGEKEY, 0644)) == -1) {
            perror("Error: msgget");
            exit(errno);
      }

     	 shmPtr = shmat(shmid, NULL, 0);

		


	
	printf("%d\n",shmPtr->clockInfo.nanoSeconds);

	srand(time(NULL));

    	int time_needed_to_execute_instructions = rand() % 40000000;

	while(1){
	
		if (msgrcv(messageQueueId, &message,sizeof(message)+1,1,0) == -1) {
			perror("msgrcv");

		}


		printf("message receive is %s, %d\n",message.mtext, some);

	}

	return 0;
}


void mail_the_message(int destAddress) {
    static int sizeOfMessage;
    Message message;
    message.messageAddress = destAddress; 
    message.returnAddress = child_id;
    sizeOfMessage = sizeof(message) - sizeof(long);
    if(msgsnd(messageQueueId, &message, sizeOfMessage, 0) == -1){
	perror(

   }	
}

void receiveMessage(int destAddress) {
    static int sizeOfMessage;
    Message message;
    sizeOfMessage = sizeof(message) - sizeof(long);
    msgrcv(messageQueueId, &message, sizeOfMessage, destAddress, 0);
}

