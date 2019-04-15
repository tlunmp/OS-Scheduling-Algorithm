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

	int position = atoi(argv[1]);



     if ((shmid = shmget(SHMKEY, sizeof(SharedObject), 0600)) < 0) {
            perror("Error: shmget");
            exit(errno);
     }
   
     if ((messageQueueId = msgget(MESSAGEKEY, 0644)) == -1) {
            perror("Error: msgget");
            exit(errno);
      }

 	 
	shmPtr = shmat(shmid, NULL, 0);

	while(1) {
	

		if (msgrcv(messageQueueId, &message,sizeof(message)+1,1,0) == -1) {
			perror("msgrcv");
		}

		int timeSlice = atoi(message.mtext);

			srand(rand()+ shmPtr->clockInfo.nanoSeconds);
        		int chance = rand() % 21;
       	


//			printf("entering critical section\n");

			if(timeSlice == chance){
				message.myType = 2;	
				strcpy(message.mtext,"Terminated");
				if(msgsnd(messageQueueId, &message,sizeof(message)+1,0) == -1) {
					perror("msgsnd");
					exit(1);
				}
			} else {

				message.myType = 2;
				char buffer1[100];
				sprintf(buffer1, "%d", chance);
				strcpy(message.mtext,buffer1);
				if(msgsnd(messageQueueId, &message,sizeof(message)+1,0) == -1) {
					perror("msgsnd");
					exit(1);
			}	
		}




	}


	return 0;
}
