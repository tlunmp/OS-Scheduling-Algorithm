#include "control.h"
#include <math.h>
#define SetBit(A,k)     ( A[((k-1)/32)] |= (1 << ((k-1)%32)) ) 
#define ClearBit(A,k)   ( A[((k-1)/32)] &= ~(1 << ((k-1)%32)) ) 
#define TestBit(A,k)    ( A[((k-1)/32)] & (1 << ((k-1)%32)) )


static char* fileName = "logfile.txt";
static int max = 10;
static int msgid;
int processTable[32];
int fakePid;
int shmid;

static pid_t actual[MAX_USER_PROCESSES + 1];
Clock newProcessCreateTime;

shared_memory_object_t* shm_ptr;
FILE* fp;

int maxProcess = 100;
int line = 1;
int terminateProcessCount = 0;
double avgTAT = 0.00;
unsigned long long totalTAT = 0;
double avgWaitTime = 0.00;
unsigned int cpu_idle_itme = 0;

int selection(void);
void setUpPCB(int position);
void scheduler(int fakePid);
void getInterval(Clock *destinationClock, Clock sourceClock, int addNanoSeconds);
int getTableIndex(int array[]);
void signalCall(int signum);
void mailMessage(int destinationAddress);
void recieveMessage(int destinationAddress);
void makeUserProcesses(int position);
void createQueueSystem();

int main(int argc, char* argv[]) {
    //generate SIGINT via Ctrl+c
    if (signal(SIGINT, signalCall) == SIG_ERR) {
        perror("Error: master: signal(): SIGINT\n");
        exit(errno);
    }

    if (signal(SIGALRM, signalCall) == SIG_ERR) {
        perror("Error: child: signal(): SIGALRM\n");
        exit(errno);
    }

    alarm(max);

    if ((shmid = shmget(SHM_KEY, sizeof(shared_memory_object_t), IPC_CREAT | 0600)) < 0) {
        perror("Error: shmget");
        exit(errno);
    }
    
    if ((msgid = msgget(MESSAGE_QUEUE_KEY, IPC_CREAT | 0600)) < 0) {
        perror("Error: mssget");
        exit(errno);
    }
    
    shm_ptr = shmat(shmid, NULL, 0);
    shm_ptr->clock_info.seconds = 0;
    shm_ptr->clock_info.nanoSeconds = 0;
    newProcessCreateTime.seconds = 0;
    newProcessCreateTime.nanoSeconds = 0;

    int proxyRide;

    createQueueSystem();

    fp = fopen(fileName, "w");

   int totalCount =0;
    while(totalCount < maxProcess) {

        if(line > LINE_MAX) {
            fclose(fp);
        }
        
      
        getInterval(&shm_ptr->clock_info, shm_ptr->clock_info, rand() % CONTEXT_SWITCH_TIME + 1);

        if ((shm_ptr->clock_info.seconds > newProcessCreateTime.seconds) || 
        (shm_ptr->clock_info.seconds == newProcessCreateTime.seconds && shm_ptr->clock_info.nanoSeconds > newProcessCreateTime.nanoSeconds)) {

           
                fakePid = getTableIndex(processTable);
                printf("\nprocess_table_reservation = %d\n", fakePid);

            if (fakePid != -1) {
                makeUserProcesses(fakePid);
		   totalCount++;                
         	   ++line;
        	    SetBit(processTable, fakePid);
        	    push_enqueue(multilevelQueue[shm_ptr->process_control_block[fakePid].priority], fakePid);

         	   getInterval(&newProcessCreateTime, shm_ptr->clock_info, rand() % 2000000000);

         	   fprintf(fp, "OSS: New process generated PID %d  at time %d:%d\n",  fakePid, shm_ptr->clock_info.seconds, shm_ptr->clock_info.nanoSeconds);
        	    ++line;
            }
        }

        proxyRide = selection();
        if(proxyRide != -1){
    
            mailMessage(proxyRide);
            fprintf(fp, "OSS: Dispatching process with PID %d  into queue %d at time %d:%d \n", proxyRide, shm_ptr->process_control_block[fakePid].priority, shm_ptr->clock_info.seconds, shm_ptr->clock_info.nanoSeconds);
            ++line;

            recieveMessage(MASTER_PROCESS_ADDRESS);
                shm_ptr->process_control_block[fakePid].process_arrives.seconds = shm_ptr->clock_info.seconds;
                shm_ptr->process_control_block[fakePid].process_arrives.nanoSeconds = shm_ptr->clock_info.nanoSeconds;
            }
        }


    avgWaitTime = cpu_idle_itme/terminateProcessCount;
    printf("\navg_wait_time = %.0f nanoSeconds per process\n", avgWaitTime);
    avgTAT = totalTAT/terminateProcessCount;
    printf("\navg_turn_around_time = %.0f nanoSeconds per process\n", avgTAT);
    printf("\nCPU_Idle_Time = %u nanoSeconds\n", cpu_idle_itme);
    msgctl(msgid, IPC_RMID, NULL);
    printf("master done\n");
    printf("\nline count = %d\n", line);
     shmdt(shm_ptr); //detaches a section of shared memory
    shmctl(shmid, IPC_RMID, NULL);  // deallocate the memory
   fclose(fp);

    return 0; 
}

void setUpPCB(int position){
	 shm_ptr->process_control_block[position].priority = 0;
        shm_ptr->process_control_block[position].cpu_usage_time = 0;
        shm_ptr->process_control_block[position].burst = 0;
        shm_ptr->process_control_block[position].time_quantum = multilevelQueue[shm_ptr->process_control_block[position].priority]->time_quantum;
        shm_ptr->process_control_block[position].finished = 0;
        shm_ptr->process_control_block[position].process_starts.seconds = shm_ptr->clock_info.seconds;
        shm_ptr->process_control_block[position].process_starts.nanoSeconds = shm_ptr->clock_info.nanoSeconds;
        shm_ptr->process_control_block[position].process_arrives.seconds = 0;
        shm_ptr->process_control_block[position].process_arrives.nanoSeconds = 0;
    
}


//int i; 
int selection(void) {
    int i; 
    int selected_process;

    for(i = 0; i < 3; ++i) {
        selected_process = pop_dequeue(multilevelQueue[i]);

         if (selected_process == -1) {
            continue;
        }

        return selected_process;
    }
    
    //Report Back a -1 If There Are No Processes In Any Queues; ALL QUEUES ARE EMPTY
    return -1;
}


void scheduler(int fakePid) {
    //Priority for user at fakePid
    int currentPriority = shm_ptr->process_control_block[fakePid].priority;

    // CPU BOUND PROCESS:
    if (shm_ptr->process_control_block[fakePid].burst == multilevelQueue[shm_ptr->process_control_block[fakePid].priority]->time_quantum) {
        
        shm_ptr->process_control_block[fakePid].priority = (currentPriority + 1 >= 3) ? currentPriority : ++currentPriority;
        shm_ptr->process_control_block[fakePid].time_quantum = multilevelQueue[currentPriority]->time_quantum;
    }

    
    else {
    
        fprintf(fp, "%OSS: PID %d not using entire time quantum\n", fakePid);
        ++line;

        currentPriority = 0;
        shm_ptr->process_control_block[fakePid].priority = currentPriority;
        shm_ptr->process_control_block[fakePid].time_quantum = multilevelQueue[currentPriority]->time_quantum;
    }
   
    
    push_enqueue(multilevelQueue[currentPriority], fakePid);

    //Output Message:
    fprintf(fp, "%OSS: Putting process with PID %d into queue %d\n",fakePid, currentPriority);
    ++line;
}


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

    
    avgWaitTime = cpu_idle_itme/terminateProcessCount;

    printf("\navg_wait_time = %.0f nanoSeconds per process\n", avgWaitTime);
    avgTAT = totalTAT/terminateProcessCount;
    printf("\navg_turn_around_time = %.0f nanoSeconds per process\n", avgTAT);
    printf("\nCPU_Idle_Time = %u nanoSeconds\n", cpu_idle_itme);
    msgctl(msgid, IPC_RMID, NULL);
    printf("\nline count = %d\n", line);
 
   kill(0, SIGTERM);
   //clean up program before exit (via interrupt signal)
    shmdt(shm_ptr); //detaches a section of shared memory
    shmctl(shmid, IPC_RMID, NULL);  // deallocate the memory
   fclose(fp);
  
      exit(EXIT_SUCCESS);
 }


void mailMessage(int destinationAddress){
    static int size_of_message;
    message_t message;
    message.message_address = destinationAddress;
    size_of_message = sizeof(message) - sizeof(message.message_address);
    msgsnd(msgid, &message, size_of_message, 0);
}

void recieveMessage(int destinationAddress){
    static int size_of_message;
    message_t message;
    size_of_message = sizeof(message) - sizeof(long);
    msgrcv(msgid, &message, size_of_message, destinationAddress, 0);

    int fakePid = message.return_address;

    printf("\nid=%d\n", message.return_address);
    fprintf(fp, "\nOSS: Receiving that process with PID %d ran for %d nanoseconds\n", fakePid,shm_ptr->clock_info.nanoSeconds);
    ++line;

    int status;
    getInterval(&shm_ptr->clock_info, shm_ptr->clock_info, shm_ptr->process_control_block[fakePid].burst);
    


    if(shm_ptr->process_control_block[fakePid].finished == 1) {
        shm_ptr->process_control_block[fakePid].wait_time = ((shm_ptr->clock_info.seconds + shm_ptr->clock_info.nanoSeconds) - (shm_ptr->process_control_block[fakePid].process_starts.seconds + shm_ptr->process_control_block[fakePid].process_starts.nanoSeconds) - shm_ptr->process_control_block[fakePid].cpu_usage_time);
      cpu_idle_itme += shm_ptr->process_control_block[fakePid].wait_time;

        shm_ptr->process_control_block[fakePid].turn_around_time = (((shm_ptr->clock_info.seconds * 1000000000) + shm_ptr->clock_info.nanoSeconds) - ((shm_ptr->process_control_block[fakePid].process_arrives.seconds * 1000000000) + shm_ptr->process_control_block[fakePid].process_arrives.nanoSeconds));
        fprintf(fp, "\n%OSS: PID %d turnaround time = %d nanoSeconds\n",fakePid, shm_ptr->process_control_block[fakePid].turn_around_time);
        ++line;

        totalTAT += shm_ptr->process_control_block[fakePid].turn_around_time;

        kill(actual[fakePid], SIGINT);
        ++terminateProcessCount;

        waitpid(actual[fakePid], &status, 0);

        ClearBit(processTable, fakePid);
        
        fprintf(fp, "OSS: PID %d terminated at time %d:%d\n", fakePid, shm_ptr->clock_info.seconds, shm_ptr->clock_info.nanoSeconds);
        ++line;
        fprintf(fp, "%OSS: PID %d burst time %d\n", fakePid, shm_ptr->process_control_block[fakePid].burst);
        ++line;
    }

    else {
        scheduler(fakePid);
        
    }
}


//search the process table
int getTableIndex(int array[]) {
   int i;
    for (i = 1; i <= MAX_USER_PROCESSES; i++) {
        if(!TestBit(array, i)) {
            printf("\ni = %d\n", i);
            return i;
        }
        printf("**PROCESS_TABLE_FULL**\n");
        return -1;
        
    }
}


void makeUserProcesses(int position) {
        char child_id[3];

	setUpPCB(position);
	actual[position] = fork();

        if (actual[position] < 0) {
            //fork failed
            perror("Fork failed");
            exit(errno);
        }
        else if (actual[position] == 0) {
            sprintf(child_id, "%d", position);
            execl("./user", "user", child_id, NULL);
            perror("User failed to execl child exe");
            printf("THIS SHOULD NEVER EXECUTE\n");
        }

    }

void createQueueSystem(){

    //Create the Multi-Level Queue System:
    int k=0;
    for (k = 0; k < 3; k++){
        multilevelQueue[k] = create_a_new_queue(TIME_QUANT_QUEUE_0 * pow(2, k)); //not
    }

    int j = 0; 
    for (j = 0; j <= 32; j++) {
        processTable[j] = 0;
    }



}
