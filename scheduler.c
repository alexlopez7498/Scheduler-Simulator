#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Headers as needed

typedef enum {false, true} bool;        // Allows boolean types in C

/* Defines a job struct */
typedef struct Process {
    uint32_t A;                         // A: Arrival time of the process
    uint32_t B;                         // B: Upper Bound of CPU burst times of the given random integer list
    uint32_t C;                         // C: Total CPU time required
    uint32_t M;                         // M: Multiplier of CPU burst time
    uint32_t processID;                 // The process ID given upon input read

    uint8_t status;                     // 0 is unstarted, 1 is ready, 2 is running, 3 is blocked, 4 is terminated

    int32_t finishingTime;              // The cycle when the the process finishes (initially -1)
    uint32_t currentCPUTimeRun;         // The amount of time the process has already run (time in running state)
    uint32_t currentIOBlockedTime;      // The amount of time the process has been IO blocked (time in blocked state)
    uint32_t currentWaitingTime;        // The amount of time spent waiting to be run (time in ready state)

    uint32_t IOBurst;                   // The amount of time until the process finishes being blocked
    uint32_t CPUBurst;                  // The CPU availability of the process (has to be > 1 to move to running)

    int32_t quantum;                    // Used for schedulers that utilise pre-emption
    int32_t orginialA;
    int32_t orginialC;
    bool isFirstTimeRunning;            // Used to check when to calculate the CPU burst when it hits running mode
    bool finished;
    struct Process* nextInBlockedList;  // A pointer to the next process available in the blocked list
    struct Process* nextInReadyQueue;   // A pointer to the next process available in the ready queue
    struct Process* nextInReadySuspendedQueue; // A pointer to the next process available in the ready suspended queue
} _process;


uint32_t CURRENT_CYCLE = 0;             // The current cycle that each process is on
uint32_t TOTAL_CREATED_PROCESSES = 0;   // The total number of processes constructed
uint32_t TOTAL_STARTED_PROCESSES = 0;   // The total number of processes that have started being simulated
uint32_t TOTAL_FINISHED_PROCESSES = 0;  // The total number of processes that have finished running
uint32_t TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0; // The total cycles in the blocked state

const char* RANDOM_NUMBER_FILE_NAME= "random-numbers.txt";
const uint32_t SEED_VALUE = 200;  // Seed value for reading from file

// Additional variables as needed


/**
 * Reads a random non-negative integer X from a file with a given line named random-numbers (in the current directory)
 */
uint32_t getRandNumFromFile(uint32_t line, FILE* random_num_file_ptr){
    uint32_t end, loop;
    char str[512];

    rewind(random_num_file_ptr); // reset to be beginning
    for(end = loop = 0;loop<line;++loop){
        if(0==fgets(str, sizeof(str), random_num_file_ptr)){ //include '\n'
            end = 1;  //can't input (EOF)
            break;
        }
    }
    if(!end) {
        return (uint32_t) atoi(str);
    }

    // fail-safe return
    return (uint32_t) 1804289383;
}



/**
 * Reads a random non-negative integer X from a file named random-numbers.
 * Returns the CPU Burst: : 1 + (random-number-from-file % upper_bound)
 */
uint32_t randomOS(uint32_t upper_bound, uint32_t process_indx, FILE* random_num_file_ptr)
{
    char str[20];
    
    uint32_t unsigned_rand_int = (uint32_t) getRandNumFromFile(SEED_VALUE+process_indx, random_num_file_ptr);
    uint32_t returnValue = 1 + (unsigned_rand_int % upper_bound);

    return returnValue;
} 


/********************* SOME PRINTING HELPERS *********************/


/**
 * Prints to standard output the original input
 * process_list is the original processes inputted (in array form)
 */
void printStart(_process process_list[])
{
    printf("The original input was: %i", TOTAL_CREATED_PROCESSES);

    uint32_t i = 0;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
    }
    printf("\n");
} 

/**
 * Prints to standard output the final output
 * finished_process_list is the terminated processes (in array form) in the order they each finished in.
 */
void printFinal(_process finished_process_list[])
{
    printf("The (sorted) input is: %i", TOTAL_CREATED_PROCESSES);
    uint32_t i = 0;
    TOTAL_FINISHED_PROCESSES = TOTAL_CREATED_PROCESSES;
    for (; i < TOTAL_FINISHED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", finished_process_list[i].A, finished_process_list[i].B,
               finished_process_list[i].C, finished_process_list[i].M);
    }
    printf("\n");
} // End of the print final function

/**
 * Prints out specifics for each process.
 * @param process_list The original processes inputted, in array form
 */
void printProcessSpecifics(_process process_list[])
{
    uint32_t i = 0;
    printf("\n");
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf("Process %i:\n", process_list[i].processID);
        printf("\t(A,B,C,M) = (%i,%i,%i,%i)\n", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
        printf("\tFinishing time: %i\n", process_list[i].finishingTime);
        printf("\tTurnaround time: %i\n", process_list[i].finishingTime - process_list[i].A);
        printf("\tI/O time: %i\n", process_list[i].currentIOBlockedTime);
        printf("\tWaiting time: %i\n", process_list[i].currentWaitingTime);
        printf("\n");
    }
} // End of the print process specifics function

/**
 * Prints out the summary data
 * process_list The original processes inputted, in array form
 */
void printSummaryData(_process process_list[])
{
    uint32_t i = 0;
    double total_amount_of_time_utilizing_cpu = 0.0;
    double total_amount_of_time_io_blocked = 0.0;
    double total_amount_of_time_spent_waiting = 0.0;
    double total_turnaround_time = 0.0;
    uint32_t final_finishing_time = CURRENT_CYCLE - 1;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        total_amount_of_time_utilizing_cpu += process_list[i].currentCPUTimeRun;
        total_amount_of_time_io_blocked += process_list[i].currentIOBlockedTime;
        total_amount_of_time_spent_waiting += process_list[i].currentWaitingTime;
        total_turnaround_time += (process_list[i].finishingTime - process_list[i].A);
    }

    // Calculates the CPU utilisation
    double cpu_util = total_amount_of_time_utilizing_cpu / final_finishing_time;

    // Calculates the IO utilisation
    double io_util = (double) TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED / final_finishing_time;

    // Calculates the throughput (Number of processes over the final finishing time times 100)
    double throughput =  100 * ((double) TOTAL_CREATED_PROCESSES/ final_finishing_time);

    // Calculates the average turnaround time
    double avg_turnaround_time = total_turnaround_time / TOTAL_CREATED_PROCESSES;

    // Calculates the average waiting time
    double avg_waiting_time = total_amount_of_time_spent_waiting / TOTAL_CREATED_PROCESSES;

    printf("Summary Data:\n");
    printf("\tFinishing time: %i\n", CURRENT_CYCLE - 1);
    printf("\tCPU Utilisation: %6f\n", cpu_util);
    printf("\tI/O Utilisation: %6f\n", io_util);
    printf("\tThroughput: %6f processes per hundred cycles\n", throughput);
    printf("\tAverage turnaround time: %6f\n", avg_turnaround_time);
    printf("\tAverage waiting time: %6f\n", avg_waiting_time);
} // End of the print summary data function

int readProcessesFromFile(FILE *input_file, _process *process_list) 
{

    if (fscanf(input_file, "%d", &TOTAL_CREATED_PROCESSES) != 1) // we fscanf the first number which is the total processes
    {
        fprintf(stderr, "Error reading number of processes\n");
        fclose(input_file);
        return 1;
    }

    for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++) // for how many processes we have we fscanf the rest of the number and assign them
    {
        fscanf(input_file, " (%d %d %d %d)", &process_list[i].A, &process_list[i].B, &process_list[i].C, &process_list[i].M);
        process_list[i].orginialA = process_list[i].A;
        process_list[i].orginialC = process_list[i].C;
        process_list[i].processID = i;
    }

    fclose(input_file);
    return 0;
}

void simulateFCFS(_process* process_list, FILE* random_num_file_ptr) 
{
    int i = 0;
    TOTAL_FINISHED_PROCESSES = 0;
    TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0;
    CURRENT_CYCLE = 0;
    int j = 0;
    int check = 0;
    int runner = 0;
    int runner2 = 0;
    printf("######################### START OF First Come First Serve #########################\n");
    printStart(process_list); // print the beginning of process list
    while(j < TOTAL_CREATED_PROCESSES) // loop through all process and set all the values to their base value
    {
        process_list[j].status = 0;
        process_list[j].finishingTime = 0;
        process_list[j].currentCPUTimeRun = 0;
        process_list[j].currentIOBlockedTime = 0;
        process_list[j].currentWaitingTime = 0;
        process_list[j].CPUBurst = randomOS(process_list[j].B,0,random_num_file_ptr);
        process_list[j].IOBurst = process_list[j].CPUBurst * process_list[j].M;
        process_list[j].nextInBlockedList = NULL;
        process_list[j].nextInReadyQueue = NULL;
        process_list[j].nextInReadySuspendedQueue = NULL;
        process_list[j].finished = false;
        j++;
    }
    j = 0; // reset j back to 0
    while(TOTAL_FINISHED_PROCESSES < TOTAL_CREATED_PROCESSES) // we loop for all processes till all of them are finished
    {
        printf(" Before cycle: %d",CURRENT_CYCLE);
        while(i < TOTAL_CREATED_PROCESSES) // we loop through all the processes in the process list every cycle
        {
            if(j < TOTAL_CREATED_PROCESSES)
            {
                if(process_list[j].status == 0) // if status is 0 we know its unstarted
                {
                    printf(" unstarted ");
                }
                else if(process_list[j].status == 1) // if status is 1 we know its ready and we increment that process waiting time
                {
                    printf(" ready ");
                    process_list[j].currentWaitingTime++;
                }
                else if(process_list[j].status == 2) // if status is 2 we know its running and we increment that process CPUTimeRun and we -- from the CPU burst and the original C
                {
                    printf(" running ");
                    process_list[i].currentCPUTimeRun++;
                    process_list[i].orginialC--;
                    process_list[j].CPUBurst--;
                }
                else if(process_list[j].status == 3) // if status is 3 we know its blocked and we increment that process blocked time and -- the IO burst
                {
                    printf(" blocked ");
                    process_list[j].currentIOBlockedTime++;
                    process_list[j].IOBurst--;

                }
                else if(process_list[j].status == 4) // if status is 4 we know its terminated
                {
                    printf(" terminated ");
                }
                printf(" %d ",process_list[j].status);
                j++;
            }
            if(process_list[i].orginialA == 0) // check if the arrival time hits 0 and we can make it ready or running
            {
                if(CURRENT_CYCLE == 0) // the first cycle 
                {
                    if(process_list[i].orginialA == 0)
                    {
                        for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++) // we loop for all the processes
                        {
                            if(process_list[k].status == 2) // if a running process is found we make the i value process in the ready status
                            {
                                process_list[i].status = 1;
                                if(runner == 0) // if this is the first process to be ready then we just put it in the ready queue
                                {
                                    process_list[i].nextInReadyQueue = &process_list[i]; 
                                }
                                runner = 1; // make runner 1 to keep a vlue telling us that a process is running
                                check = 1; // check is used to make it a runner or not
                            }
                        }
                        if(check == 0) // if there was no runners then we make it a run
                        {
                            process_list[i].status = 2;
                        }
                    }
                }
                else if(process_list[i].status == 0) // if the process is unstarted we make it ready and place it into ready queue
                {
                    process_list[i].status = 1;
                    process_list[i].nextInReadyQueue = &process_list[i];
                }
                else if(process_list[i].status == 2 && process_list[i].CPUBurst == 0) // if process is running and CPU burst is 0 then we go to block status and generate a new CPU burst
                {
                    process_list[i].status = 3;
                    process_list[i].CPUBurst = randomOS(process_list[i].B,0,random_num_file_ptr);
                    if(process_list[i].orginialC == 0)
                    {

                    }
                    else
                    {
                        process_list[i].nextInReadySuspendedQueue = &process_list[i];
                    }
                }
                else if(process_list[i].nextInReadyQueue != NULL) // we the process is in ready queue
                {
                    for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++) // check if theres process running
                    {
                        if(process_list[k].status == 2)
                        {
                            runner = 1;
                        }
                    }
                    if(i == 0 && process_list[TOTAL_CREATED_PROCESSES - 1].status == 2 && process_list[i].IOBurst == 0)
                    { // if we are at the first process and the last process is running and its IO burst is 0 then we make the first process running
                        process_list[i].status = 2;
                        process_list[i].IOBurst = process_list[i].CPUBurst * process_list[i].M; // generate a new IO burst
                        runner = 1;
                        process_list[i].nextInReadyQueue = NULL;
                    }
                    else if(i == 0 && process_list[TOTAL_CREATED_PROCESSES - 1].status != 2) // checks to make sure its not the else statement
                    {

                    }
                    else if(runner == 1)
                    {

                    }
                    else
                    {
                        process_list[i].status = 2;
                        process_list[i].IOBurst = process_list[i].CPUBurst * process_list[i].M;
                        runner = 1;
                        process_list[i].nextInReadyQueue = NULL;
                    }
                }
                else if(runner == 1 && process_list[i].status == 1) // if theres a runner and the process is ready then we check if the process before is running
                {
                    if(process_list[i - 1].status == 2)
                    {
                        process_list[i].nextInReadyQueue = &process_list[i];
                    }
                }
                else if(process_list[i].nextInReadySuspendedQueue != NULL && process_list[i].IOBurst == 0) // we process was in suspened queue then we make can take to the ready to be up next to run
                {
                    process_list[i].status = 1;
                    process_list[i].nextInReadySuspendedQueue = NULL;
                    process_list[i].nextInReadyQueue = &process_list[i];
                }
                if(process_list[i].orginialC == 0 && process_list[i].finished == false) // if the cpu completion time hits 0 and it wasn't finished already then we termiate it
                {
                    process_list[i].status = 4;
                    process_list[i].finished = true;
                    process_list[i].finishingTime = CURRENT_CYCLE;
                    TOTAL_FINISHED_PROCESSES++;

                }

            }
            else // if arrival time isn't 0 then we -- till it's 0 because right now its unstarted
            {
                process_list[i].orginialA--;
                
            }
            i++;
        }
        for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++) // check to see if theres a running process at the end
        {
            if(process_list[k].status == 2)
            {
                runner2 = 1;
            }
        }
        if(runner2 == 0) // if theres not and theres a process in the ready queue then we make that a running process
        {
            for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++)
            {
                if(process_list[k].nextInReadyQueue != NULL && runner2 == 0)
                {
                    process_list[k].nextInReadyQueue = NULL;
                    process_list[k].status = 2;
                    process_list[k].IOBurst = process_list[k].CPUBurst * process_list[k].M;
                    runner2 = 1;
                }
            }
        }
        i = 0;
        printf("\n");
        j = 0;
        CURRENT_CYCLE++; // increment cycle at the end
        runner = 0; // and reset the variables for the next cycle
        runner2 = 0;
    }
    for(int i = 0; i < TOTAL_FINISHED_PROCESSES; i++) // we also add the blocked time to number of cycles spent blocked
    {
        TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED += process_list[i].currentIOBlockedTime;
    }
    printProcessSpecifics(process_list); // print final specifics and summary
    printSummaryData(process_list);
    printFinal(process_list);
    printf("######################### END OF First Come First Serve #########################\n");
    return;
}

void simulateRR(_process* process_list, FILE* random_num_file_ptr) 
{
    int i = 0;
    TOTAL_FINISHED_PROCESSES = 0;
    TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0;
    CURRENT_CYCLE = 0;
    int j = 0;
    int check = 0;
    int runner = 0;
    int runner2 = 0;
    printf("######################### START OF ROUND ROBIN #########################\n");
    printStart(process_list); // print the beginning of process list
    while(j < TOTAL_CREATED_PROCESSES) // loop through all process and set all the values to their base value
    {
        process_list[j].status = 0;
        process_list[j].finishingTime = 0;
        process_list[j].currentCPUTimeRun = 0;
        process_list[j].currentIOBlockedTime = 0;
        process_list[j].currentWaitingTime = 0;
        process_list[j].quantum = 2;
        process_list[j].orginialC = process_list[j].C;
        process_list[j].orginialA = process_list[j].A;
        process_list[j].CPUBurst = randomOS(process_list[j].B,0,random_num_file_ptr);
        process_list[j].IOBurst = process_list[j].CPUBurst * process_list[j].M;
        process_list[j].nextInBlockedList = NULL;
        process_list[j].nextInReadyQueue = NULL;
        process_list[j].nextInReadySuspendedQueue = NULL;
        process_list[j].finished = false;
        j++;
    }
    j = 0; // reset j back to 0
    while(TOTAL_FINISHED_PROCESSES < TOTAL_CREATED_PROCESSES) // we loop for all processes till all of them are finished
    {
        printf(" Before cycle: %d",CURRENT_CYCLE);
        while(i < TOTAL_CREATED_PROCESSES) // we loop through all the processes in the process list every cycle
        {
            if(j < TOTAL_CREATED_PROCESSES)
            {
                if(process_list[j].status == 0) // if status is 0 we know its unstarted
                {
                    printf(" unstarted ");
                }
                else if(process_list[j].status == 1) // if status is 1 we know its ready and we increment that process waiting time
                {
                    printf(" ready ");
                    process_list[j].currentWaitingTime++;
                }
                else if(process_list[j].status == 2) // if status is 2 we know its running and we increment that process CPUTimeRun and we -- from the CPU burst and the original C
                {
                    printf(" running ");
                    process_list[i].currentCPUTimeRun++;
                    process_list[i].orginialC--;
                    process_list[j].CPUBurst--;
                    process_list[j].quantum--;
                }
                else if(process_list[j].status == 3) // if status is 3 we know its blocked and we increment that process blocked time and -- the IO burst
                {
                    printf(" blocked ");
                    process_list[j].currentIOBlockedTime++;
                    process_list[j].IOBurst--;

                }
                else if(process_list[j].status == 4) // if status is 4 we know its terminated
                {
                    printf(" terminated ");
                }
                printf(" %d ",process_list[j].status);
                j++;
            }
            if(process_list[i].orginialA == 0) // check if the arrival time hits 0 and we can make it ready or running
            {
                if(CURRENT_CYCLE == 0) // the first cycle 
                {
                    if(process_list[i].orginialA == 0)
                    {
                        for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++) // we loop for all the processes
                        {
                            if(process_list[k].status == 2) // if a running process is found we make the i value process in the ready status
                            {
                                process_list[i].status = 1;
                                if(runner == 0) // if this is the first process to be ready then we just put it in the ready queue
                                {
                                    process_list[i].nextInReadyQueue = &process_list[i];
                                }
                                runner = 1; // make runner 1 to keep a vlue telling us that a process is running
                                check = 1; // check is used to make it a runner or not
                            }
                        }
                        if(check == 0) // if there was no runners then we make it a run
                        {
                            process_list[i].status = 2;
                        }
                    }
                }
                else if(process_list[i].status == 0) // if the process is unstarted we make it ready and place it into ready queue
                {
                    process_list[i].status = 1;
                    process_list[i].nextInReadyQueue = &process_list[i];
                }
                else if(process_list[i].status == 2 && process_list[i].CPUBurst == 0) // if process is running and CPU burst is 0 then we go to block status and generate a new CPU burst
                {
                    process_list[i].status = 3;
                    process_list[i].quantum = 2;
                    process_list[i].CPUBurst = randomOS(process_list[i].B,0,random_num_file_ptr);
                    if(process_list[i].orginialC == 0)
                    {

                    }
                    else
                    {
                        process_list[i].nextInReadySuspendedQueue = &process_list[i];
                    }
                }
                else if(process_list[i].nextInReadyQueue != NULL) // we the process is in ready queue
                {
                    for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++) // check if theres process running
                    {
                        if(process_list[k].status == 2)
                        {
                            runner = 1;
                        }
                    }
                    if(i == 0 && process_list[TOTAL_CREATED_PROCESSES - 1].status == 2 && process_list[i].IOBurst == 0)
                    { // if we are at the first process and the last process is running and its IO burst is 0 then we make the first process running
                        process_list[i].status = 2;
                        process_list[i].IOBurst = process_list[i].CPUBurst * process_list[i].M; // generate a new IO burst
                        runner = 1;
                        process_list[i].nextInReadyQueue = NULL;
                    }
                    else if(i == 0 && process_list[TOTAL_CREATED_PROCESSES - 1].status != 2) // checks to make sure its not the else statement
                    {

                    }
                    else if(runner == 1)
                    {

                    }
                    else
                    {
                        process_list[i].status = 2;
                        process_list[i].IOBurst = process_list[i].CPUBurst * process_list[i].M;
                        runner = 1;
                        process_list[i].nextInReadyQueue = NULL;
                    }
                }
                else if(runner == 1 && process_list[i].status == 1) // if theres a runner and the process is ready then we check if the process before is running
                {
                    if(process_list[i - 1].status == 2)
                    {
                        process_list[i].nextInReadyQueue = &process_list[i];
                    }
                }
                else if(process_list[i].nextInReadySuspendedQueue != NULL && process_list[i].IOBurst == 0) // when process was in suspened queue then we make can take to the ready to be up next to run
                {
                    process_list[i].status = 1;
                    process_list[i].nextInReadySuspendedQueue = NULL;
                    process_list[i].nextInReadyQueue = &process_list[i];
                }
                if(process_list[i].orginialC == 0 && process_list[i].finished == false) // if the cpu completion time hits 0 and it wasn't finished already then we termiate it
                {
                    process_list[i].status = 4;
                    process_list[i].finished = true;
                    process_list[i].finishingTime = CURRENT_CYCLE;
                    TOTAL_FINISHED_PROCESSES++;

                }
                if(process_list[i].quantum == 0 && process_list[i].orginialC != 0) // check if the quantum is 0 then we stop running 
                {
                    process_list[i].quantum = 2;
                    process_list[i].status = 1;
                    process_list[i].nextInReadyQueue = &process_list[i];
                }
            }
            else // if arrival time isn't 0 then we -- till it's 0 because right now its unstarted
            {
                process_list[i].orginialA--;
            }
            i++;
        }
        for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++) // check to see if theres a running process at the end
        {
            if(process_list[k].status == 2)
            {
                runner2 = 1;
            }
        }
        if(runner2 == 0) // if theres not and theres a process in the ready queue then we make that a running process
        {
            for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++)
            {
                if(process_list[k].nextInReadyQueue != NULL && runner2 == 0)
                {
                    process_list[k].nextInReadyQueue = NULL;
                    process_list[k].status = 2;
                    if(process_list[k].IOBurst == 0)
                    {
                        process_list[k].IOBurst = process_list[k].CPUBurst * process_list[k].M;
                    }
                    runner2 = 1;
                }
            }
        }
        i = 0;
        printf("\n");
        j = 0;
        CURRENT_CYCLE++; // increment cycle at the end
        runner = 0; // and reset the variables for the next cycle
        runner2 = 0;
    }
    for(int i = 0; i < TOTAL_CREATED_PROCESSES; i++) // we also add the blocked time to number of cycles spent blocked
    {
        TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED += process_list[i].currentIOBlockedTime;
    }

    printProcessSpecifics(process_list); // print final specifics and summary
    printSummaryData(process_list);
    printFinal(process_list);
    printf("######################### END OF ROUND ROBIN #########################\n");
    return;
}

void simulateSJF(_process* process_list, FILE* random_num_file_ptr) 
{
    int i = 0;
    TOTAL_FINISHED_PROCESSES = 0;
    TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0;
    CURRENT_CYCLE = 0;
    int j = 0;
    int readyCheck = 0;
    int minRunner = 10000;
    int saveIndex = 0;
    int check = 0;
    int runner = 0;
    int runner2 = 0;
    printf("######################### START OF SHORTEST JOB FIRST #########################\n");
    printStart(process_list); // print the beginning of process list
    while(j < TOTAL_CREATED_PROCESSES) // loop through all process and set all the values to their base value
    {
        process_list[j].status = 0;
        process_list[j].finishingTime = 0;
        process_list[j].currentCPUTimeRun = 0;
        process_list[j].currentIOBlockedTime = 0;
        process_list[j].currentWaitingTime = 0;
        process_list[j].orginialC = process_list[j].C;
        process_list[j].orginialA = process_list[j].A;
        process_list[j].CPUBurst = randomOS(process_list[j].B,0,random_num_file_ptr);
        process_list[j].IOBurst = process_list[j].CPUBurst * process_list[j].M;
        process_list[j].nextInBlockedList = NULL;
        process_list[j].nextInReadyQueue = NULL;
        process_list[j].nextInReadySuspendedQueue = NULL;
        process_list[j].finished = false;
        j++;
    }
    j = 0; // reset j back to 0
    while(TOTAL_FINISHED_PROCESSES < TOTAL_CREATED_PROCESSES) // we loop for all processes till all of them are finished
    {
        printf(" Before cycle: %d",CURRENT_CYCLE);
        while(i < TOTAL_CREATED_PROCESSES) // we loop through all the processes in the process list every cycle
        {
            if(j < TOTAL_CREATED_PROCESSES)
            {
                if(process_list[j].status == 0) // if status is 0 we know its unstarted
                {
                    printf(" unstarted ");
                }
                else if(process_list[j].status == 1) // if status is 1 we know its ready and we increment that process waiting time
                {
                    printf(" ready ");
                    process_list[j].currentWaitingTime++;
                }
                else if(process_list[j].status == 2) // if status is 2 we know its running and we increment that process CPUTimeRun and we -- from the CPU burst and the original C
                {
                    printf(" running ");
                    process_list[i].currentCPUTimeRun++;
                    process_list[i].orginialC--;
                    process_list[j].CPUBurst--;
                }
                else if(process_list[j].status == 3) // if status is 3 we know its blocked and we increment that process blocked time and -- the IO burst
                {
                    printf(" blocked ");
                    process_list[j].currentIOBlockedTime++;
                    process_list[j].IOBurst--;

                }
                else if(process_list[j].status == 4) // if status is 4 we know its terminated
                {
                    printf(" terminated ");
                }
                printf(" %d ",process_list[j].status);
                j++;
            }
            if(process_list[i].orginialA == 0) // check if the arrival time hits 0 and we can make it ready or running
            {
                if(CURRENT_CYCLE == 0) // the first cycle 
                {
                    if(process_list[i].orginialA == 0)
                    {
                        for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++) // we loop for all the processes
                        {
                            if(process_list[k].status == 2) // if a running process is found we make the i value process in the ready status
                            {
                                process_list[i].status = 1;
                                if(runner == 0) // if this is the first process to be ready then we just put it in the ready queue
                                {
                                    process_list[i].nextInReadyQueue = &process_list[i];
                                }
                                runner = 1; // make runner 1 to keep a vlue telling us that a process is running
                                check = 1; // check is used to make it a runner or not
                            }
                        }
                        if(check == 0) // if there was no runners then we make it a run
                        {
                            process_list[i].status = 2;
                        }
                    }
                }
                else if(process_list[i].status == 0) // if the process is unstarted we make it ready and place it into ready queue
                {
                    process_list[i].status = 1;
                    process_list[i].nextInReadyQueue = &process_list[i];
                }
                else if(process_list[i].status == 2 && process_list[i].CPUBurst == 0) // if process is running and CPU burst is 0 then we go to block status and generate a new CPU burst
                {
                    process_list[i].status = 3;
                    process_list[i].CPUBurst = randomOS(process_list[i].B,0,random_num_file_ptr);
                    if(process_list[i].orginialC == 0)
                    {

                    }
                    else
                    {
                        process_list[i].nextInReadySuspendedQueue = &process_list[i];
                    }
                }
                else if(process_list[i].nextInReadyQueue != NULL && process_list[i].status != 4) // we the process is in ready queue
                {
                    for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++) // check if theres process running
                    {
                        if(process_list[k].status == 2)
                        {
                            runner = 1;
                        }
                    }
                    if(i == 0 && process_list[TOTAL_CREATED_PROCESSES - 1].status == 2 && process_list[i].IOBurst == 0)
                    { // if we are at the first process and the last process is running and its IO burst is 0 then we make the first process running
                        process_list[i].status = 2;
                        process_list[i].IOBurst = process_list[i].CPUBurst * process_list[i].M; // generate a new IO burst
                        runner = 1;
                        process_list[i].nextInReadyQueue = NULL;
                    }
                    else if(i == 0 && process_list[TOTAL_CREATED_PROCESSES - 1].status != 2) // checks to make sure its not the else statement
                    {

                    }
                    else if(runner == 1)
                    {

                    }
                    else if(process_list[i].status == 4)
                    {

                    }
                    else
                    {
                        process_list[i].status = 2;
                        process_list[i].IOBurst = process_list[i].CPUBurst * process_list[i].M;
                        runner = 1;
                        process_list[i].nextInReadyQueue = NULL;
                    }
                }
                else if(runner == 1 && process_list[i].status == 1 && process_list[i].status != 4) // if theres a runner and the process is ready then we check if the process before is running
                {
                    if(process_list[i - 1].status == 2)
                    {
                        process_list[i].nextInReadyQueue = &process_list[i];
                    }
                }
                else if(process_list[i].nextInReadySuspendedQueue != NULL && process_list[i].IOBurst == 0 && process_list[i].status != 4) // we process was in suspened queue then we make can take to the ready to be up next to run
                {
                    process_list[i].status = 1;
                    process_list[i].nextInReadySuspendedQueue = NULL;
                    process_list[i].nextInReadyQueue = &process_list[i];
                    for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++)
                    {
                        if(process_list[k].status == 1)
                        {
                            readyCheck++;
                        }
                    }
                    if(readyCheck >= 2)
                    {
                        minRunner = process_list[i].orginialC;
                        saveIndex = i;
                        for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++)
                        {
                            if(process_list[k].status != 4)
                            {
                                if(process_list[k].orginialC + 1 < minRunner)
                                {
                                    saveIndex = k;
                                    minRunner = process_list[k].orginialC;
                                }
                            }
                        }
                        process_list[saveIndex].status = 2;
                        process_list[i].IOBurst = process_list[i].CPUBurst * process_list[i].M;
                        runner = 1;
                        process_list[i].nextInReadyQueue = NULL;
                    }
                }
                if(process_list[i].orginialC == 0 && process_list[i].finished == false) // if the cpu completion time hits 0 and it wasn't finished already then we termiate it
                {
                    process_list[i].status = 4;
                    process_list[i].finished = true;
                    process_list[i].nextInReadyQueue = NULL;
                    process_list[i].nextInReadySuspendedQueue = NULL;
                    process_list[i].finishingTime = CURRENT_CYCLE;
                    TOTAL_FINISHED_PROCESSES++;

                }

            }
            else // if arrival time isn't 0 then we -- till it's 0 because right now its unstarted
            {
                process_list[i].orginialA--;
                
            }
            i++;
        }
        for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++) // check to see if theres a running process at the end
        {
            if(process_list[k].status == 2)
            {
                runner2 = 1;
            }
        }
        if(runner2 == 0) // if theres not and theres a process in the ready queue then we make that a running process
        {
            for(int k = 0; k < TOTAL_CREATED_PROCESSES; k++)
            {
                if(process_list[k].nextInReadyQueue != NULL && runner2 == 0)
                {
                    process_list[k].nextInReadyQueue = NULL;
                    process_list[k].status = 2;
                    process_list[k].IOBurst = process_list[k].CPUBurst * process_list[k].M;
                    runner2 = 1;
                }
            }
        }
        i = 0;
        printf("\n");
        j = 0;
        CURRENT_CYCLE++; // increment cycle at the end
        runner = 0; // and reset the variables for the next cycle
        readyCheck = 0;
        minRunner = 0;
        saveIndex = 0;
        runner2 = 0;

    }
    for(int i = 0; i < TOTAL_FINISHED_PROCESSES; i++) // we also add the blocked time to number of cycles spent blocked
    {
        TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED += process_list[i].currentIOBlockedTime;
    }
    printProcessSpecifics(process_list);  // print final specifics and summary
    printSummaryData(process_list);
    printFinal(process_list);
    printf("######################### END OF SHORTEST JOB FIRST #########################\n");
    return;
}
/**
 * The magic starts from here
 */

int main(int argc, char *argv[]) 
{
    _process *process_list = malloc(100 * sizeof(_process));
    FILE *random_num_file_ptr = fopen(RANDOM_NUMBER_FILE_NAME, "r");
    char *input_file_path = argv[1];
    FILE *input_file = fopen(input_file_path, "r");
    readProcessesFromFile(input_file, process_list);
    simulateFCFS(process_list, random_num_file_ptr);

    simulateRR(process_list, random_num_file_ptr);

    simulateSJF(process_list, random_num_file_ptr);
    fclose(random_num_file_ptr);

    if(process_list != NULL)
    {
        free(process_list);
    }
    return 0;
}
