#include "control.h"
#include <math.h>
/*getopt() flags:*/
static int h_flag = 0;
static char* fileName = "logfile.txt";
static int max = 20;

static int message_queue_id;
static pid_t actual[MAX_USER_PROCESSES + 1];
system_clock_t newProcessCreateTime;
int process_table[32];
int process_table_reservation;
int shmid;
shared_memory_object_t* shm_ptr;
FILE* fp;


int line_counter = 1;
int terminated_process_count = 0;
double avg_turn_around_time = 0.00;
unsigned long long total_turn_around_time = 0;
double avg_wait_time = 0.00;
unsigned int cpu_idle_itme = 0;

int process_selection(void);
void setUpPCB(int process_table_position);
void process_scheduler(int process_table_reservation);

void clock_incrementation_function(system_clock_t *destinationClock, system_clock_t sourceClock, int additional_nano_seconds);


int detachandremove (int shmid, void* shmaddr){
    int error = 0;
    if (shmdt(shmaddr) == - 1)
        error = errno;
    if ((shmctl(shmid, IPC_RMID, NULL) == -1) && !error)
        error = errno;
    if (!error)
        return 0;
    errno = error;
    return -1;
}


void makeUserProcesses(int process_table_position) {
        char child_id[3];

	setUpPCB(process_table_position);       
	actual[process_table_position] = fork();

        if (actual[process_table_position] < 0) {
            //fork failed
            perror("Fork failed");
            exit(errno);
        }
        else if (actual[process_table_position] == 0) {
            sprintf(child_id, "%d", process_table_position);
            execl("./user", "user", child_id, NULL);
            perror("User failed to execl child exe");
            printf("THIS SHOULD NEVER EXECUTE\n");
        }

    }

//master.c signal handler for master process
void signalCallback (int signum) {
    int j, status;

    if (signum == SIGINT)
        printf("\nSIGINT received by master\n");
    else
        printf("\nSIGALRM received by master\n");

   for (j = 1; j <= MAX_USER_PROCESSES; j++){ // !!!!!!!!
        kill(actual[j], SIGINT);
    }

   while(wait(&status) > 0);
   
   //Remove All Queues
   int i;
   for (i = 0; i < 3; ++i) {
       destroyQueue(multilevel_queue_system[i]);
   }

    avg_wait_time = cpu_idle_itme/terminated_process_count;
    printf("\navg_wait_time = %.0f nano_seconds per process\n", avg_wait_time);
    avg_turn_around_time = total_turn_around_time/terminated_process_count;
    printf("\navg_turn_around_time = %.0f nano_seconds per process\n", avg_turn_around_time);
    printf("\nCPU_Idle_Time = %u nano_seconds\n", cpu_idle_itme);
    detachandremove(shmid,shm_ptr);
    msgctl(message_queue_id, IPC_RMID, NULL);
    printf("master done\n");
    printf("\nline count = %d\n", line_counter);
    fclose(fp);
    exit(0);
}

void mail_the_message(int destination_address){
    static int size_of_message;
    message_t message;
    message.message_address = destination_address;
    size_of_message = sizeof(message) - sizeof(message.message_address);
    msgsnd(message_queue_id, &message, size_of_message, 0);
}

void receive_the_message(int destination_address){
    static int size_of_message;
    message_t message;
    size_of_message = sizeof(message) - sizeof(long);
    msgrcv(message_queue_id, &message, size_of_message, destination_address, 0);

    int process_table_reservation = message.return_address;

    printf("\nid=%d\n", message.return_address);
    fprintf(fp, "\nOSS: Receiving that process with PID %d ran for %d nanoseconds\n", process_table_reservation,shm_ptr->clock_info.nano_seconds);
    ++line_counter;

    int status;
    clock_incrementation_function(&shm_ptr->clock_info, shm_ptr->clock_info, shm_ptr->process_control_block[process_table_reservation].burst);
    


    if(shm_ptr->process_control_block[process_table_reservation].finished == 1) {
        shm_ptr->process_control_block[process_table_reservation].wait_time = ((shm_ptr->clock_info.seconds + shm_ptr->clock_info.nano_seconds) - (shm_ptr->process_control_block[process_table_reservation].process_starts.seconds + shm_ptr->process_control_block[process_table_reservation].process_starts.nano_seconds) - shm_ptr->process_control_block[process_table_reservation].cpu_usage_time);                     
      cpu_idle_itme += shm_ptr->process_control_block[process_table_reservation].wait_time;

        shm_ptr->process_control_block[process_table_reservation].turn_around_time = (((shm_ptr->clock_info.seconds * 1000000000) + shm_ptr->clock_info.nano_seconds) - ((shm_ptr->process_control_block[process_table_reservation].process_arrives.seconds * 1000000000) + shm_ptr->process_control_block[process_table_reservation].process_arrives.nano_seconds));
        fprintf(fp, "\n%OSS: PID %d turnaround time = %d nano_seconds\n",process_table_reservation, shm_ptr->process_control_block[process_table_reservation].turn_around_time);
        ++line_counter;
        total_turn_around_time += shm_ptr->process_control_block[process_table_reservation].turn_around_time;
        kill(actual[process_table_reservation], SIGINT);
        ++terminated_process_count;
        waitpid(actual[process_table_reservation], &status, 0);
        ClearBit(process_table, process_table_reservation);
        
        fprintf(fp, "OSS: PID %d terminated at time %d:%d\n", process_table_reservation, shm_ptr->clock_info.seconds, shm_ptr->clock_info.nano_seconds);
        ++line_counter;
        fprintf(fp, "%OSS: PID %d burst time %d\n", process_table_reservation, shm_ptr->process_control_block[process_table_reservation].burst);
        ++line_counter;
    }

    else {
        process_scheduler(process_table_reservation);
        
    }

}

//search the process table
int get_process_table_index_state(int array[]) {
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

void printUsage()
{
    printf("Usage: oss -l <log_file.txt> -t <20>\n");
}



int main(int argc, char* argv[]) {
    //generate SIGINT via Ctrl+c
    if (signal(SIGINT, signalCallback) == SIG_ERR) {
        perror("Error: master: signal(): SIGINT\n");
        exit(errno);
    }
    if (signal(SIGALRM, signalCallback) == SIG_ERR) {
        perror("Error: child: signal(): SIGALRM\n");
        exit(errno);
    }


    alarm(max);

    if ((shmid = shmget(SHM_KEY, sizeof(shared_memory_object_t), IPC_CREAT | 0600)) < 0) {
        perror("Error: shmget");
        exit(errno);
    }
    
    if ((message_queue_id = msgget(MESSAGE_QUEUE_KEY, IPC_CREAT | 0600)) < 0) {
        perror("Error: mssget");
        exit(errno);
    }
    
    shm_ptr = shmat(shmid, NULL, 0);
    shm_ptr->clock_info.seconds = 0;
    shm_ptr->clock_info.nano_seconds = 0;
    newProcessCreateTime.seconds = 0;
    newProcessCreateTime.nano_seconds = 0;

    int proxyRide;

    //Create the Multi-Level Queue System:
    int k=0;
    for (k = 0; k < 3; k++){
        multilevel_queue_system[k] = create_a_new_queue(TIME_QUANT_QUEUE_0 * pow(2, k)); //not
    }

    int j = 0; 
    for (j = 0; j <= 32; j++) {
        process_table[j] = 0;
    }

    fp = fopen(fileName, "w");

   
    while(1) {

        if(line_counter > LINE_MAX) {
            fclose(fp);
        }
        
      
        clock_incrementation_function(&shm_ptr->clock_info, shm_ptr->clock_info, rand() % CONTEXT_SWITCH_TIME + 1);

        if ((shm_ptr->clock_info.seconds > newProcessCreateTime.seconds) || 
        (shm_ptr->clock_info.seconds == newProcessCreateTime.seconds && shm_ptr->clock_info.nano_seconds > newProcessCreateTime.nano_seconds)) {

           
                process_table_reservation = get_process_table_index_state(process_table);
                printf("\nprocess_table_reservation = %d\n", process_table_reservation);

            if (process_table_reservation != -1) {
                makeUserProcesses(process_table_reservation);
                
            ++line_counter;
            SetBit(process_table, process_table_reservation);
            push_enqueue(multilevel_queue_system[shm_ptr->process_control_block[process_table_reservation].priority], process_table_reservation);

            clock_incrementation_function(&newProcessCreateTime, shm_ptr->clock_info, rand() % 2000000000);

            fprintf(fp, "OSS: New process generated PID %d  at time %d:%d\n",  process_table_reservation, shm_ptr->clock_info.seconds, shm_ptr->clock_info.nano_seconds);
            ++line_counter;
            }
        }
        proxyRide = process_selection();
        if(proxyRide != -1){
    
            mail_the_message(proxyRide); 
            fprintf(fp, "OSS: Dispatching process with PID %d  into queue %d at time %d:%d \n", proxyRide, shm_ptr->process_control_block[process_table_reservation].priority, shm_ptr->clock_info.seconds, shm_ptr->clock_info.nano_seconds);
            ++line_counter;

            receive_the_message(MASTER_PROCESS_ADDRESS);
                shm_ptr->process_control_block[process_table_reservation].process_arrives.seconds = shm_ptr->clock_info.seconds;
                shm_ptr->process_control_block[process_table_reservation].process_arrives.nano_seconds = shm_ptr->clock_info.nano_seconds;
            }
        }


    return 0; 
}

void setUpPCB(int process_table_position){
	 shm_ptr->process_control_block[process_table_position].priority = 0;
        shm_ptr->process_control_block[process_table_position].cpu_usage_time = 0;
        shm_ptr->process_control_block[process_table_position].burst = 0;
        shm_ptr->process_control_block[process_table_position].time_quantum = multilevel_queue_system[shm_ptr->process_control_block[process_table_position].priority]->time_quantum;
        shm_ptr->process_control_block[process_table_position].finished = 0;
        shm_ptr->process_control_block[process_table_position].process_starts.seconds = shm_ptr->clock_info.seconds;
        shm_ptr->process_control_block[process_table_position].process_starts.nano_seconds = shm_ptr->clock_info.nano_seconds;
        shm_ptr->process_control_block[process_table_position].process_arrives.seconds = 0;
        shm_ptr->process_control_block[process_table_position].process_arrives.nano_seconds = 0;
    



}


//int i; 
int process_selection(void) {
    int i; 
    int selected_process;

//3 = the number of queues in our multilevel queue system
    for(i = 0; i < 3; ++i) {
        selected_process = pop_dequeue(multilevel_queue_system[i]);

         if (selected_process == -1) {
            continue;
        }

        return selected_process;
    }
    
    //Report Back a -1 If There Are No Processes In Any Queues; ALL QUEUES ARE EMPTY
    return -1;
}


void process_scheduler(int process_table_reservation) {
    //Priority for user at process_table_reservation
    int current_priority = shm_ptr->process_control_block[process_table_reservation].priority;

    // CPU BOUND PROCESS:
    if (shm_ptr->process_control_block[process_table_reservation].burst == multilevel_queue_system[shm_ptr->process_control_block[process_table_reservation].priority]->time_quantum) {
        
        shm_ptr->process_control_block[process_table_reservation].priority = (current_priority + 1 >= 3) ? current_priority : ++current_priority;
        shm_ptr->process_control_block[process_table_reservation].time_quantum = multilevel_queue_system[current_priority]->time_quantum;
    }

    
    else {
    
        fprintf(fp, "%OSS: PID %d not using entire time quantum\n", process_table_reservation);
        ++line_counter;

        current_priority = 0;
        shm_ptr->process_control_block[process_table_reservation].priority = current_priority;
        shm_ptr->process_control_block[process_table_reservation].time_quantum = multilevel_queue_system[current_priority]->time_quantum;
    }
   
    
    push_enqueue(multilevel_queue_system[current_priority], process_table_reservation);

    //Output Message:
    fprintf(fp, "%OSS: Putting process with PID %d into queue %d\n",process_table_reservation, current_priority);
    ++line_counter;
}


void clock_incrementation_function(system_clock_t *destinationClock, system_clock_t sourceClock, int additional_nano_seconds) {
 
    destinationClock->nano_seconds = sourceClock.nano_seconds + additional_nano_seconds;
    if(destinationClock->nano_seconds > 1000000000) {
        destinationClock->seconds++;
        destinationClock->nano_seconds -= 1000000000;
    }
  
}


