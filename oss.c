#include "control.h"


char outputFileName[] = "logFile.txt";
static int timer = 10;
static int msgid;
static pid_t actual[MAX_USER_PROCESSES + 1];
Clock maxTimeBetweenProc;

SharedMemory* shm_ptr;
FILE* fp;

int maxProcess = 100;
int line = 1;
int terminateProcessCount = 0;
int queueLevel = 4;
int processTable[32];
int fakePid;
int shmid;

double avgTAT = 0.00;
unsigned long long totalTAT = 0;
double avgWaitTime = 0.00;
unsigned int cpuIdleTime = 0;

void helpMenu();
int selection(void);
void setUpPCB(int position);
void userOrReal(int fakePid);
void getInterval(Clock *destinationClock, Clock sourceClock, int addNanoSeconds);
int getTableIndex(int array[]);
void signalCall(int signum);
void mailMessage(int destinationAddress);
void recieveMessage(int destinationAddress);
void makeUserProcesses(int position);
void createQueueSystem();

int main(int argc, char* argv[]) {
	int c;
	//getopt command for command line
	while((c = getopt (argc,argv, "ho:")) != -1) {

		switch(c) {
			case 'h':
				helpMenu();
				return 1;
			case 'o':
				strcpy(outputFileName, optarg);
				break;
			default:
				fprintf(stderr, "%s: Error: Unknown option -%c\n",argv[0],optopt);
				return -1;	
		}


	}





    //generate SIGINT via Ctrl+c
    if (signal(SIGINT, signalCall) == SIG_ERR) {
        perror("Error: master: signal(): SIGINT\n");
        exit(errno);
    }

    if (signal(SIGALRM, signalCall) == SIG_ERR) {
        perror("Error: child: signal(): SIGALRM\n");
        exit(errno);
    }

   //alarm timer
   alarm(timer);

	//share the key 
    if ((shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0600)) < 0) {
        perror("Error: shmget");
        exit(errno);
    }

	//get message key    
    if ((msgid = msgget(MESSAGEKEY, IPC_CREAT | 0600)) < 0) {
        perror("Error: mssget");
        exit(errno);
    }
    
    shm_ptr = shmat(shmid, NULL, 0);
    shm_ptr->clockInfo.seconds = 0;
    shm_ptr->clockInfo.nanoSeconds = 0;
    maxTimeBetweenProc.seconds = 0;
    maxTimeBetweenProc.nanoSeconds = 0;

    int pidSelection;

    createQueueSystem();

    fp = fopen(outputFileName, "w");

   int totalCount =0;
    while(totalCount < maxProcess) {
    
	//getinterval from the start random 
        getInterval(&shm_ptr->clockInfo, shm_ptr->clockInfo, rand() % CONSTANTNUMBER + 1);

        if(line > LINE_MAX) {
            fclose(fp);
        }
        
 
	//check the time to launch compare to the launch time
        if ((shm_ptr->clockInfo.seconds > maxTimeBetweenProc.seconds) ||
        (shm_ptr->clockInfo.seconds == maxTimeBetweenProc.seconds && shm_ptr->clockInfo.nanoSeconds > maxTimeBetweenProc.nanoSeconds)) {

           
                fakePid = getTableIndex(processTable);

		//fakepid is not -1 then create new process
            if (fakePid != -1) {
                makeUserProcesses(fakePid);
		        totalCount++;
                ++line;
        	    
		//set the bit then push the the queue in mulitlevel
                SetBit(processTable, fakePid);
        	    pushEnqueue(multilevelQueue[shm_ptr->processCB[fakePid].priority], fakePid);
			

		//increase interval for new process to generate
         	   getInterval(&maxTimeBetweenProc, shm_ptr->clockInfo, rand() % CONSTANTNUMBER + 1);

         	   fprintf(fp, "OSS: Generating process with PID %d  at time %d:%d\n",  fakePid, shm_ptr->clockInfo.seconds, shm_ptr->clockInfo.nanoSeconds);
        	    ++line;
            }
        }

        //get the queue selection
        pidSelection = selection();
    
        //if its empty
        if(pidSelection != -1){
    
            //send message
            mailMessage(pidSelection);
            fprintf(fp, "OSS: Dispatching process with PID %d  into queue %d at time %d:%d \n", pidSelection, shm_ptr->processCB[fakePid].priority, shm_ptr->clockInfo.seconds, shm_ptr->clockInfo.nanoSeconds);
            ++line;

		//return the address so master knows who got it
            recieveMessage(ADDRESS);
                shm_ptr->processCB[fakePid].process_arrives.seconds = shm_ptr->clockInfo.seconds;
                shm_ptr->processCB[fakePid].process_arrives.nanoSeconds = shm_ptr->clockInfo.nanoSeconds;
            }
        }


    //end show the information
    avgWaitTime = cpuIdleTime/terminateProcessCount;
    fprintf(stderr,"Average Wait Time = %.0f nanoSeconds per process\n", avgWaitTime);
    avgTAT = totalTAT/terminateProcessCount;
    fprintf(stderr,"Average Turn Around Time = %.0f nanoSeconds per process\n", avgTAT);
    fprintf(stderr,"CPU Idle Time = %u nanoSeconds\n", cpuIdleTime);
    msgctl(msgid, IPC_RMID, NULL);
    fprintf(stderr,"\nline count = %d\n", line);
     shmdt(shm_ptr); //detaches a section of shared memory
    shmctl(shmid, IPC_RMID, NULL);  // deallocate the memory
   fclose(fp);

    return 0; 
}


//setup the process control table to record stats of the burst time
void setUpPCB(int position){
	 shm_ptr->processCB[position].priority = 0;
        shm_ptr->processCB[position].cpu_usage_time = 0;
        shm_ptr->processCB[position].burst = 0;
        shm_ptr->processCB[position].time_quantum = multilevelQueue[shm_ptr->processCB[position].priority]->time_quantum;
        shm_ptr->processCB[position].finished = 0;
        shm_ptr->processCB[position].process_starts.seconds = shm_ptr->clockInfo.seconds;
        shm_ptr->processCB[position].process_starts.nanoSeconds = shm_ptr->clockInfo.nanoSeconds;
        shm_ptr->processCB[position].process_arrives.seconds = 0;
        shm_ptr->processCB[position].process_arrives.nanoSeconds = 0;
    
}


//slectint the process
int selection(void) {
    int i; 
    int selectedProcess;

  
    for(i = 0; i < queueLevel; ++i) {
        selectedProcess = pop_dequeue(multilevelQueue[i]);

         if (selectedProcess == -1) {
            continue;
        }

        return selectedProcess;
    }
   
	//if theres no queue 
    return -1;
}


void userOrReal(int fakePid) {
    //Priority for user
    int currentPriority = shm_ptr->processCB[fakePid].priority;

    //or real priority
    if (shm_ptr->processCB[fakePid].burst == multilevelQueue[shm_ptr->processCB[fakePid].priority]->time_quantum) {
        
        shm_ptr->processCB[fakePid].priority = (currentPriority + 1 >= 3) ? currentPriority : ++currentPriority;
        shm_ptr->processCB[fakePid].time_quantum = multilevelQueue[currentPriority]->time_quantum;
    }

    
    else {
    
        fprintf(fp, "OSS: PID %d not using entire time quantum\n", fakePid);
        ++line;

	//set priority to 0
        currentPriority = 0;
        shm_ptr->processCB[fakePid].priority = currentPriority;
        shm_ptr->processCB[fakePid].time_quantum = multilevelQueue[currentPriority]->time_quantum;
    }
   
    //put it in the queue 
    pushEnqueue(multilevelQueue[currentPriority], fakePid);

    //Output Message:
    fprintf(fp, "OSS: Putting process with PID %d into queue %d\n",fakePid, currentPriority);
    ++line;
}


//incrementing the clock
void getInterval(Clock *destinationClock, Clock sourceClock, int addNanoSeconds) {
    destinationClock->nanoSeconds = sourceClock.nanoSeconds + addNanoSeconds;
    if(destinationClock->nanoSeconds > 1000000000) {
        destinationClock->seconds++;
        destinationClock->nanoSeconds -= 1000000000;
    }
  
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


    //print it out the statitistics
    avgWaitTime = cpuIdleTime/terminateProcessCount;
    fprintf(stderr,"Average Wait Time = %.0f nanoSeconds per process\n", avgWaitTime);
    avgTAT = totalTAT/terminateProcessCount;
    fprintf(stderr,"Average Turn Around Time = %.0f nanoSeconds per process\n", avgTAT);
    fprintf(stderr,"CPU Idle Time = %u nanoSeconds\n", cpuIdleTime);
    msgctl(msgid, IPC_RMID, NULL);
    fprintf(stderr,"\nline count = %d\n", line);
 
   kill(0, SIGTERM);
   //clean up program before exit (via interrupt signal)
    shmdt(shm_ptr); //detaches a section of shared memory
    shmctl(shmid, IPC_RMID, NULL);  // deallocate the memory
   fclose(fp);
  
      exit(EXIT_SUCCESS);
 }

//mailing message from master to user
void mailMessage(int destinationAddress){
    static int size_of_message;
    Message message;
    message.messageAddress = destinationAddress;
    size_of_message = sizeof(message) - sizeof(message.messageAddress);
    msgsnd(msgid, &message, size_of_message, 0);
}

//recieve message from the user.c if it terminates or not, or be block
void recieveMessage(int destinationAddress){
    static int size_of_message;
    Message message;
    size_of_message = sizeof(message) - sizeof(long);
    msgrcv(msgid, &message, size_of_message, destinationAddress, 0);

    int fakePid = message.returnAddress;

   
    fprintf(fp, "OSS: Receiving that process with PID %d ran for %d nanoseconds\n", fakePid,shm_ptr->clockInfo.nanoSeconds);
    ++line;

    int status;
    getInterval(&shm_ptr->clockInfo, shm_ptr->clockInfo, shm_ptr->processCB[fakePid].burst);
    

	//if it terminates assign and calculate the 
    if(shm_ptr->processCB[fakePid].finished == 1) {
        shm_ptr->processCB[fakePid].wait_time = ((shm_ptr->clockInfo.seconds + shm_ptr->clockInfo.nanoSeconds) - (shm_ptr->processCB[fakePid].process_starts.seconds + shm_ptr->processCB[fakePid].process_starts.nanoSeconds) - shm_ptr->processCB[fakePid].cpu_usage_time);
      cpuIdleTime += shm_ptr->processCB[fakePid].wait_time;

	//get turn around time
        shm_ptr->processCB[fakePid].turn_around_time = (((shm_ptr->clockInfo.seconds * 1000000000) + shm_ptr->clockInfo.nanoSeconds) - ((shm_ptr->processCB[fakePid].process_arrives.seconds * 1000000000) + shm_ptr->processCB[fakePid].process_arrives.nanoSeconds));
        fprintf(fp, "OSS: PID %d turnaround time = %d nanoSeconds\n",fakePid, shm_ptr->processCB[fakePid].turn_around_time);
        ++line;

	//get the total turn around time
        totalTAT += shm_ptr->processCB[fakePid].turn_around_time;

        kill(actual[fakePid], SIGINT);
        ++terminateProcessCount;

        waitpid(actual[fakePid], &status, 0);

        ClearBit(processTable, fakePid);

	//print show the termination and burst time        
        fprintf(fp, "OSS: PID %d terminated at time %d:%d\n", fakePid, shm_ptr->clockInfo.seconds, shm_ptr->clockInfo.nanoSeconds);
        ++line;
        fprintf(fp, "OSS: PID %d burst time %d\n", fakePid, shm_ptr->processCB[fakePid].burst);
        ++line;
    }

//if not terminated then schedule another one
    else {
        userOrReal(fakePid);
        
    }
}


//search the process table
int getTableIndex(int array[]) {
   int i;
    for (i = 1; i <= MAX_USER_PROCESSES; i++) {
        if(!TestBit(array, i)) {
            return i;
        }
        return -1;
        
    }
}


//will make use process and fork it
void makeUserProcesses(int position) {
        char childId[3];

	//setup control block
	setUpPCB(position);
	pid_t childpid = fork();
	actual[position] = childpid;

        if (childpid < 0) {
            //fork failed
            perror("Fork failed");
            exit(errno);
        }

        else if (childpid == 0) {
            sprintf(childId, "%d", position);
            execl("./user", "user", childId, NULL);
            perror("Error");
        
        }

    }

void createQueueSystem(){

    //Create the Multi-Level Queue System:
    int k=0;
    for (k = 0; k < 4; k++){
        multilevelQueue[k] = createNewQueue(TIME_QUANTUM * 2); 
    }

    int j = 0; 
    for (j = 0; j <= 32; j++) {
        processTable[j] = 0;
    }



}
//help menu
void helpMenu() {
		printf("---------------------------------------------------------------| Help Menu |--------------------------------------------------------------------------\n");
		printf("-h help menu\n"); 
		printf("-o inputfilename                      | ouputfilename of the log file\n"); 
		printf("------------------------------------------------------------------------------------------------------------------------------------------------------\n");
}

