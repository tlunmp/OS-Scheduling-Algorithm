#include "control.h"

int child_id;
int shmid;
shared_memory_object_t* shm_ptr;
static int message_queue_id;

void signalCallback (int signum) {
    printf("\nSIGTERM received by user_process_id %d\n", child_id);
    shmdt(shm_ptr);
    exit(0);
}

void mail_the_message(int destination_address) {
    static int size_of_message;
    message_t message;
    message.message_address = destination_address;
    message.return_address = child_id;
    size_of_message = sizeof(message) - sizeof(long);
    msgsnd(message_queue_id, &message, size_of_message, 0);
}

void receive_the_message(int destination_address) {
    static int size_of_message;
    message_t message;
    size_of_message = sizeof(message) - sizeof(long);
    msgrcv(message_queue_id, &message, size_of_message, destination_address, 0);
}

int main(int argc, char* argv[])
{

    child_id = atoi(argv[1]);
	
    if (signal(SIGINT, signalCallback) == SIG_ERR) {
        perror("Error: slave: signal().\n");
        exit(errno);
    }

   
    if ((shmid = shmget(SHM_KEY, sizeof(shared_memory_object_t), 0600)) < 0) {
        perror("Error: shmget");
        exit(errno);
    }

   
    if ((message_queue_id = msgget(MESSAGE_QUEUE_KEY, 0600)) < 0) {
        perror("Error: msgget");
        exit(errno);
    }

   
    shm_ptr = shmat(shmid, NULL, 0);
   
    srand(time(NULL));

    int time_needed_to_execute_instructions = rand() % 40000000; //this determines how much time each process will need to run for to "finish" it's job



    while(1){
        receive_the_message(child_id);
        
        int was_time_quantum_used_up = rand() % 1000;
   
        if (was_time_quantum_used_up == 0) {
            shm_ptr->process_control_block[child_id].burst = shm_ptr->process_control_block[child_id].time_quantum;
        } else {
            shm_ptr->process_control_block[child_id].burst = rand() % shm_ptr->process_control_block[child_id].time_quantum;
        }

        int execution_time_remaining = time_needed_to_execute_instructions - shm_ptr->process_control_block[child_id].cpu_usage_time;
        if (shm_ptr->process_control_block[child_id].burst > execution_time_remaining) {
            shm_ptr->process_control_block[child_id].burst = execution_time_remaining;
        }

        shm_ptr->process_control_block[child_id].cpu_usage_time += shm_ptr->process_control_block[child_id].burst;
        
        if (shm_ptr->process_control_block[child_id].cpu_usage_time >= time_needed_to_execute_instructions) {
            shm_ptr->process_control_block[child_id].finished = 1;
        }

        mail_the_message(MASTER_PROCESS_ADDRESS);
        printf("\nchild_id=%d\n", child_id);
    }

    return 0;
}
