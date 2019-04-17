#include "control.h"

int childId;
int shmid;
SharedMemory* shm_ptr;
static int msgId;


void signalCallback (int signum);
void mailMessage(int destination_address, char* termMessage);
void recieveMessage(int destination_address);

int main(int argc, char* argv[])
{

    childId = atoi(argv[1]);


	//signals	
    if (signal(SIGINT, signalCallback) == SIG_ERR) {
        perror("Error: slave: signal().\n");
        exit(errno);
    }

	//get the key shmget share memory
    if ((shmid = shmget(SHM_KEY, sizeof(SharedMemory), 0600)) < 0) {
        perror("Error: shmget");
        exit(errno);
    }

   	//get the message queue key to access
    if ((msgId = msgget(MESSAGEKEY, 0600)) < 0) {
        perror("Error: msgget");
        exit(errno);
    }

   
    shm_ptr = shmat(shmid, NULL, 0);
   
    srand(time(NULL));

    int timeExecution = rand() % 40000000;

    char *termMessage;

    while(1){

	//recieve from user
        recieveMessage(childId);
        
	//chance for termination probability
        int chance = rand() % 1000;
   
	//if chance not terminate
        if (chance == 0) {
            shm_ptr->processCB[childId].burst = shm_ptr->processCB[childId].time_quantum;
        } else {
            shm_ptr->processCB[childId].burst = rand() % shm_ptr->processCB[childId].time_quantum;
        }

	//chance is block then store the value, to burst
        int execRemaining = timeExecution - shm_ptr->processCB[childId].cpu_usage_time;
        if (shm_ptr->processCB[childId].burst > execRemaining) {
            shm_ptr->processCB[childId].burst = execRemaining;
            termMessage = "blocked";
        }

        shm_ptr->processCB[childId].cpu_usage_time += shm_ptr->processCB[childId].burst;
        
	//if it is terminated then set a message says terminated
        if (shm_ptr->processCB[childId].cpu_usage_time >= timeExecution) {
            shm_ptr->processCB[childId].finished = 1;
            termMessage = "terminated";
        }

	//send message to the user to master if it is terminated or not
        mailMessage(MASTER_PROCESS_ADDRESS, termMessage);
    }

    return 0;
}

//signal call backs
void signalCallback (int signum) {
    fprintf(stderr, "Generating Log File\n");
    //printf("\nSIGTERM received by User Process of %d\n", childId);
    shmdt(shm_ptr);
    exit(0);
}

//send mail message queue
void mailMessage(int destination_address,char *termMessage) {
    static int size_of_message;
    Message message;
    message.messageAddress = destination_address;
    message.returnAddress = childId;
    size_of_message = sizeof(message) - sizeof(long);
    msgsnd(msgId, &message, size_of_message, 0);
}

//if recieve message
void recieveMessage(int destination_address) {
    static int size_of_message;
    Message message;
    size_of_message = sizeof(message) - sizeof(long);
    msgrcv(msgId, &message, size_of_message, destination_address, 0);
}
