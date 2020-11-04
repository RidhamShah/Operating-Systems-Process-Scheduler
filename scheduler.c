#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum {false, true} bool;        // Allows boolean types in C

/* Defines a job struct */
struct Process {
    u_int32_t A;                         // A: Arrival time of the process
    u_int32_t B;                         // B: Upper Bound of CPU burst times of the given random integer list
    u_int32_t C;                         // C: Total CPU time required
    u_int32_t M;                         // M: Multiplier of CPU burst time
    u_int32_t processID;                 // The process ID given upon input read

    u_int8_t status;                     // 0 is unstarted, 1 is ready, 2 is running, 3 is blocked, 4 is terminated

    int32_t finishingTime;              // The cycle when the the process finishes (initially -1)
    u_int32_t currentCPUTimeRun;         // The amount of time the process has already run (time in running state)
    u_int32_t currentIOBlockedTime;      // The amount of time the process has been IO blocked (time in blocked state)
    u_int32_t currentWaitingTime;        // The amount of time spent waiting to be run (time in ready state)

    u_int32_t IOBurst;                   // The amount of time until the process finishes being blocked
    u_int32_t CPUBurst;                  // The CPU availability of the process (has to be > 1 to move to running)

    int32_t quantum;                    // Used for schedulers that utilise pre-emption {agri vastu}

    bool isFirstTimeRunning;            // Used to check when to calculate the CPU burst when it hits running mode

    struct Process* nextInBlockedList;  // A pointer to the next process available in the blocked list
    struct Process* nextInReadyQueue;   // A pointer to the next process available in the ready queue
    struct Process* nextInReadySuspendedQueue; // A pointer to the next process available in the ready suspended queue
};

/* Global values */
// Flags to be set
bool IS_VERBOSE_MODE = false;           // Flags whether the output should be detailed or not
bool IS_RANDOM_MODE = false;            // Flags whether the output should include the random digit or not
bool IS_FIRST_TIME_RUNNING_UNIPROGRAMMED = true;
struct Process* UNIPROGRAMMED_PROCESS = NULL;

u_int32_t CURRENT_CYCLE = 0;             // The current cycle that each process is on
u_int32_t TOTAL_CREATED_PROCESSES = 0;   // The total number of processes constructed
u_int32_t TOTAL_STARTED_PROCESSES = 0;   // The total number of processes that have started being simulated
u_int32_t TOTAL_FINISHED_PROCESSES = 0;  // The total number of processes that have finished running
u_int32_t TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0;

const char* RANDOM_NUMBER_FILE_NAME= "random-numbers";

/* Queue & List pointers */
// readyQueue head & tail pointers
struct Process* readyHead = NULL;
struct Process* readyTail = NULL;
u_int32_t readyProcessQueueSize = 0;

// readySuspendedQueue head & tail pointers
struct Process* readySuspendedHead = NULL;
struct Process* readySuspendedTail = NULL;
u_int32_t readySuspendedProcessQueueSize = 0;

// blockedList head & tail pointers
struct Process* blockedHead = NULL;
struct Process* blockedTail = NULL;
u_int32_t blockedProcessListSize = 0;

// finished processes pointer
struct Process* CURRENT_RUNNING_PROCESS = NULL;

/**
 * Reads a random non-negative integer X from a file named random-numbers (in the current directory)
 * NOTE: Only works if the integer values in random-number are less than 20 in length
 * @return The CPUBurst, calculated with the function: 1 + (randomNumberFromFile % UpperBound)
 */
u_int32_t randomOS(u_int32_t upperBound, FILE* randomNumberFile)
{
    char str[20];
    u_int32_t unsignedRandomInteger = (u_int32_t) atoi(fgets(str, 20, randomNumberFile)); // take random no in order of file
    u_int32_t returnValue = 1 + (unsignedRandomInteger % upperBound);
    //printf("--------debug-------%i\n",unsignedRandomInteger);
    return returnValue;
} // End of the randomOS function

/************************ START OF READY QUEUE HELPER FUNCTIONS *************************************/

/**
* A queue insertion function for the ready function
*/
void enqueueReadyProcess(struct Process* newNode)
{
    // Identical to the insertBack() of a linked list
    if (readyProcessQueueSize == 0)
    {
        // Queue is empty, simply point head and tail to the newNode
        readyHead = newNode;
        readyTail = newNode;
    }
    else
    {
        // Queue is not empty, gets the back and inserts behind the tail
        newNode->nextInReadyQueue = NULL;

        readyTail->nextInReadyQueue = newNode;
        readyTail = readyTail->nextInReadyQueue; // Sets the new tail.nextInReady == NULL
    }
    ++readyProcessQueueSize;
} // End of the ready process enqueue function

/**
 * Dequeues the process from the queue, and returns the removed node
 */
struct Process* dequeueReadyProcess()
{
    // Identical to removeFront() of a linked list
    if (readyProcessQueueSize == 0)
    {
        // Queue is empty, returns null
        printf("ERROR: Attempted to dequeue from the ready process pool\n");
        return NULL;
    }
    else
    {
        // Queue is not empty, retains the old head for the return value, and sets the new head
        struct Process* oldHead = readyHead;
        readyHead = readyHead->nextInReadyQueue;
        --readyProcessQueueSize;

        // Queue is now empty, with both head & tail set to NULL
        if (readyProcessQueueSize == 0)
            readyTail = NULL;
        oldHead->nextInReadyQueue = NULL;
        return oldHead;
    }
} // End of the ready process dequeue function

/************************ END OF READY QUEUE HELPER FUNCTIONS *************************************/

/************************ START OF READY SUSPENDED QUEUE HELPER FUNCTIONS *************************************/

/**
* A queue insertion function for the ready suspended queue
*/
void enqueueReadySuspendedProcess(struct Process* newNode)
{
    // Identical to the insertBack() of a linked list
    if (readySuspendedProcessQueueSize == 0)
    {
        // Queue is empty, simply point head and tail to the newNode
        readySuspendedHead = newNode;
        readySuspendedTail = newNode;
    }
    else
    {
        // Queue is not empty, gets the back and inserts behind the tail
        newNode->nextInReadySuspendedQueue = NULL;

        readySuspendedTail->nextInReadySuspendedQueue = newNode;
        readySuspendedTail = readySuspendedTail->nextInReadySuspendedQueue; // Sets the new tail.nextInreadySuspended == NULL
    }
    ++readySuspendedProcessQueueSize;
} // End of the readySuspended process enqueue function

/**
 * Dequeues the process from the queue, and returns the removed node
 */
struct Process* dequeueReadySuspendedProcess()
{
    // Identical to removeFront() of a linked list
    if (readySuspendedProcessQueueSize == 0)
    {
        // Queue is empty, returns null
        printf("ERROR: Attempted to dequeue from the readySuspended process pool\n");
        return NULL;
    }
    else
    {
        // Queue is not empty, retains the old head for the return value, and sets the new head
        struct Process* oldHead = readySuspendedHead;
        readySuspendedHead = readySuspendedHead->nextInReadySuspendedQueue;
        --readySuspendedProcessQueueSize;

        // Checks if queue is now empty, with both head & tail set to NULL
        if (readySuspendedProcessQueueSize == 0)
            readySuspendedTail = NULL;

        oldHead->nextInReadySuspendedQueue = NULL;
        return oldHead;
    }
} // End of the readySuspended process dequeue function

/************************ END OF READY SUSPENDED QUEUE HELPER FUNCTIONS *************************************/

/************************ START OF BLOCKED LIST HELPER FUNCTIONS *************************************/

/**
* A queue insertion function for the blocked list
*/
void addToBlockedList(struct Process* newNode)
{
    // Identical to the insertBack() of a linked list
    if ((blockedHead == NULL) || (blockedTail == NULL))
    {
        // Queue is empty, simply point head and tail to the newNode
        blockedHead = newNode;
        blockedTail = newNode;
    }
    else
    {
        // Queue is not empty, gets the back and inserts behind the tail
        blockedTail->nextInBlockedList = newNode;
        blockedTail = newNode;
    }
    ++blockedProcessListSize;
} // End of the blocked process enqueue function

/**
 * Dequeues the process from the queue, and returns the removed node
 */
struct Process* dequeueBlockedProcess()
{
    // Identical to removeFront() of a linked list
    if (blockedHead == NULL)
    {
        printf("ERROR: Attempted to dequeue from the blocked process pool\n");
        // Queue is empty, returns null
        return NULL;
    }
    else
    {
        // Queue is not empty, retains the old head for the return value, and sets the new head
        struct Process* oldHead = blockedHead;
        blockedHead = blockedHead->nextInBlockedList;
        if (blockedHead == NULL)
            blockedTail = NULL;
        --blockedProcessListSize;
        oldHead->nextInBlockedList = NULL;
        return oldHead;
    }
} // End of the blocked process dequeue function

/************************ END OF BLOCKED LIST HELPER FUNCTIONS *************************************/

/************************ START OF RUNNING PROGRAM FUNCTIONS *************************************/

/**
 * Processes any job in the blockedProcessesList
 */
void doBlockedProcesses()
{
    if (blockedProcessListSize != 0)
    {
        // Blocked list is not empty, goes through each one to check if they need to be moved to ready or not
        struct Process* currentNode = blockedHead;
        while (currentNode != NULL)
        {
            // Iterates through the blocked process list
            if ( (int32_t) currentNode->IOBurst <= 0)
            {
                // IOBurst time is 0, moves to ready
                currentNode->status = 1;

                // Removes this process from the blocked list to the ready queue
                if (currentNode->processID == blockedHead->processID)
                {
                    // Removes from the front, simply dequeues
                    struct Process* unBlockedProcess = dequeueBlockedProcess();
                    enqueueReadyProcess(unBlockedProcess);
                }
                else if (currentNode->processID == blockedTail->processID) 
                {
                    // Removes from the back
                    struct Process* currentBlockedIterationProcess = blockedHead;
                    while (currentBlockedIterationProcess->nextInBlockedList->nextInBlockedList != NULL) //head.next.next
                    {currentBlockedIterationProcess = currentBlockedIterationProcess->nextInBlockedList;}

                    blockedTail = currentBlockedIterationProcess;

                    struct Process* nodeToBeAddedToReady = currentBlockedIterationProcess->nextInBlockedList;
                    currentBlockedIterationProcess->nextInBlockedList = NULL;
                    nodeToBeAddedToReady->nextInBlockedList = NULL;
                    --blockedProcessListSize;

                    enqueueReadyProcess(nodeToBeAddedToReady);
                }
                else
                {
                    // Removes from the middle
                    struct Process* currentBlockedIterationProcess = blockedHead;

                    // Iterates until right before the middle block
                    while (currentBlockedIterationProcess->nextInBlockedList->processID != currentNode->processID)
                    {currentBlockedIterationProcess = currentBlockedIterationProcess->nextInBlockedList;}

                    struct Process* nodeToBeAddedToReady = currentBlockedIterationProcess->nextInBlockedList;

                    currentBlockedIterationProcess->nextInBlockedList =
                            currentBlockedIterationProcess->nextInBlockedList->nextInBlockedList;
                    nodeToBeAddedToReady->nextInBlockedList = NULL;
                    --blockedProcessListSize;
                    enqueueReadyProcess(nodeToBeAddedToReady);
                }
            } // End of dealing with removing any processes that need to be moved to ready
            currentNode = currentNode->nextInBlockedList;
        } // End of iterating through the blocked queue
    } // End of dealing with all blocked processes in the blocked list
} // End of the doBlockedProcess function

/**
 * Processes any job that is currently running
 * @param finishedProcessContainer The terminated processes, in array form, and in the order they each finished in.
 * @param schedulerAlgorithm Which scheduler algorithm this function should run. 0 = FCFS, 1 = RR, 2 = UNI, 3 = SJF.
 */
void doRunningProcesses(struct Process finishedProcessContainer[], u_int8_t schedulerAlgorithm)
{
    if (CURRENT_RUNNING_PROCESS != NULL)
    {
        // A process is currently running

        // Calculates the IOburst the first time around
        if (CURRENT_RUNNING_PROCESS->isFirstTimeRunning == true)
        {
            CURRENT_RUNNING_PROCESS->isFirstTimeRunning = false;
            CURRENT_RUNNING_PROCESS->IOBurst = 1 + (CURRENT_RUNNING_PROCESS->M * CURRENT_RUNNING_PROCESS->CPUBurst);
        }

        if (CURRENT_RUNNING_PROCESS->C == CURRENT_RUNNING_PROCESS->currentCPUTimeRun)
        {
            // Process has completed running, moves to finished process container
            CURRENT_RUNNING_PROCESS->status = 4;
            CURRENT_RUNNING_PROCESS->finishingTime = CURRENT_CYCLE;
            finishedProcessContainer[TOTAL_FINISHED_PROCESSES] = *CURRENT_RUNNING_PROCESS;
            ++TOTAL_FINISHED_PROCESSES;
            if (schedulerAlgorithm == 2)
                UNIPROGRAMMED_PROCESS = NULL;
            CURRENT_RUNNING_PROCESS = NULL;
        }
        else if (CURRENT_RUNNING_PROCESS->CPUBurst <= 0)
        {
            // Process has run out of CPU burst, moves to blocked
            CURRENT_RUNNING_PROCESS->status = 3;
            addToBlockedList(CURRENT_RUNNING_PROCESS);
            CURRENT_RUNNING_PROCESS = NULL;
        } // End of dealing with the running process that has run out of CPU Burst, moved to blocked list
        else if ((schedulerAlgorithm == 1) && (CURRENT_RUNNING_PROCESS->quantum <= 0))
        {
            // Process has been preempted, moves to ready
            CURRENT_RUNNING_PROCESS->status = 1;
            enqueueReadyProcess(CURRENT_RUNNING_PROCESS);
            CURRENT_RUNNING_PROCESS = NULL;
        } // End of dealing with the running process being pre-empted back to the ready queue
        else
        {
            // Process still has CPU burst, stays in running
            CURRENT_RUNNING_PROCESS->status = 2;
        }// End of dealing with the running process remaining in the running pool
    } // End of dealing with the running queue when a process is running
} // End of the do running process function

/**
 * Starts any process that begins at their designated start time (their A value)
 * @param processContainer The original processes inputted, in array form
 */
void createProcesses(struct Process processContainer[])
{
    u_int32_t i = 0;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        if (processContainer[i].A == CURRENT_CYCLE)
        {
            // Time for this process to be created, and enqueued to the ready queue
            ++TOTAL_STARTED_PROCESSES;
            processContainer[i].status = 1; // Sets the status to ready
            if ((UNIPROGRAMMED_PROCESS == NULL) && (IS_FIRST_TIME_RUNNING_UNIPROGRAMMED == true))
            {
                IS_FIRST_TIME_RUNNING_UNIPROGRAMMED = false;
                UNIPROGRAMMED_PROCESS = &processContainer[i];
            }
            enqueueReadyProcess(&processContainer[i]);
        }
    }
} // End of the createProcess function

/**
 * Processes any job in the readyQueue or readySuspendedQueue
 * @param schedulerAlgorithm Which scheduler algorithm this function should run. 0 = FCFS, 1 = RR, 2 = UNI, 3 = SJF.
 * @param currentPassNumber The current pass this simulation is run (either pass 1 or 2 for each algorithm)
 * @param randomFile The file to retrieve a "random" number to be used in calculating CPU Burst
 */
void doReadyProcesses(u_int8_t schedulerAlgorithm, u_int8_t currentPassNumber, FILE* randomFile)
{
    // Suspends anything that isn't the UNIPROGRAMMED process
    if ((UNIPROGRAMMED_PROCESS != NULL) && (readyProcessQueueSize != 0)
        && (readyHead != UNIPROGRAMMED_PROCESS) && (schedulerAlgorithm == 2))
    {
        // There is a process running, so suspends anything to the ready suspended queue
        u_int32_t i = 0;
        for (; i < readyProcessQueueSize; ++i)
        {
            struct Process* suspendedNode = dequeueReadyProcess();
            suspendedNode->status = 1;
            enqueueReadySuspendedProcess(suspendedNode);
        }
    }

    // Deals with the ready suspended queue
    if ((readySuspendedProcessQueueSize != 0) && (schedulerAlgorithm == 2))
    {
        if (UNIPROGRAMMED_PROCESS == NULL) {
            // There is no process running, dequeues a single process and readies it
            struct Process *resumedProcess = dequeueReadySuspendedProcess();
            resumedProcess->status = 1;
            UNIPROGRAMMED_PROCESS = resumedProcess;
            enqueueReadyProcess(resumedProcess);
        }
    }// End of dealing with the ready suspended queue

    // Deals with the ready queue second
    if (readyProcessQueueSize != 0)
    {
        if (CURRENT_RUNNING_PROCESS == NULL)
        {
            // No process is running, is able to pick a process to run
            if (schedulerAlgorithm == 3)
            {
                // Scheduler is shortest job first, meaning in lowest remaining CPU time required
                u_int32_t i = 0;
                struct Process* currentReadyProcess = readyHead;
                struct Process* shortestJobProcess = readyHead;
                for (; i < readyProcessQueueSize; ++i) // minimum sodhe che ---> mn = min(mn,i-j);
                {
                    if ((shortestJobProcess->C - shortestJobProcess->currentCPUTimeRun) >
                            (currentReadyProcess->C - currentReadyProcess->currentCPUTimeRun))
                    {
                        // Old shortest job is greater than the new one, sets the shortest job to point to the new one
                        shortestJobProcess = currentReadyProcess;
                    }
                    currentReadyProcess = currentReadyProcess->nextInReadyQueue;
                }// End of iterating through to find the shortest time remaining

                // Needs to deal with removing the shortest job from the ready list (from front, back, and middle)
                struct Process* readiedProcess;

                if (readyHead->processID == shortestJobProcess->processID)
                {
                    // Dequeueing from the head of ready (normal dequeue)
                    readiedProcess = dequeueReadyProcess();
                }
                else if (readyTail->processID == shortestJobProcess->processID)
                {
                    // Dequeues from the tail of ready list
                    struct Process* currentReadyIterationProcess = readyHead;
                    while (currentReadyIterationProcess->nextInReadyQueue->nextInReadyQueue != NULL)
                    {currentReadyIterationProcess = currentReadyIterationProcess->nextInReadyQueue;}

                    readyTail = currentReadyIterationProcess;

                    readiedProcess = currentReadyIterationProcess->nextInReadyQueue;
                    currentReadyIterationProcess->nextInReadyQueue = NULL;
                    readiedProcess->nextInReadyQueue = NULL;
                    --readyProcessQueueSize;
                }
                else
                {
                    // Dequeues from the middle of ready list
                    struct Process* currentReadyIterationProcess = readyHead;

                    // Iterates until right before the middle block
                    while (currentReadyIterationProcess->nextInReadyQueue->processID != shortestJobProcess->processID)
                    {currentReadyIterationProcess = currentReadyIterationProcess->nextInReadyQueue;}

                    readiedProcess = currentReadyIterationProcess->nextInReadyQueue;

                    currentReadyIterationProcess->nextInReadyQueue =
                            currentReadyIterationProcess->nextInReadyQueue->nextInReadyQueue;
                    readiedProcess->nextInReadyQueue = NULL;
                    --readyProcessQueueSize;
                }

                // At this point, we have the shortest job process that should be sent, so sets it running
                readiedProcess->status = 2;
                readiedProcess->isFirstTimeRunning = true;

                char str[20];
                u_int32_t unsignedRandomInteger = (u_int32_t) atoi(fgets(str, 20, randomFile));
                // Prints out the random number, assuming the random flag is passed in
                if ((IS_RANDOM_MODE) && (currentPassNumber % 2 == 0))
                    printf("Find burst when choosing ready process to run %i\n", unsignedRandomInteger);

                u_int32_t newCPUBurst = 1 + (unsignedRandomInteger % shortestJobProcess->B);
                // Checks if the new CPU Burst time is greater than the time remaining
                if (newCPUBurst > (readiedProcess->C - readiedProcess->currentCPUTimeRun))  
                    newCPUBurst = readiedProcess->C - readiedProcess->currentCPUTimeRun;
                readiedProcess->CPUBurst = newCPUBurst;
                CURRENT_RUNNING_PROCESS = readiedProcess;
            } // End of dealing with shortest job first
            else if( schedulerAlgorithm == 4 )
            {
                // Scheduler is shortest job first, meaning in lowest remaining CPU time required
                u_int32_t i = 0;
                struct Process* currentReadyProcess = readyHead;
                struct Process* shortestJobProcess = readyHead;
                for (; i < readyProcessQueueSize; ++i) // minimum sodhe che ---> mn = min(mn,i-j);
                {
                    if ((shortestJobProcess->C - shortestJobProcess->currentCPUTimeRun) <
                            (currentReadyProcess->C - currentReadyProcess->currentCPUTimeRun))
                    {
                        // Old shortest job is greater than the new one, sets the shortest job to point to the new one
                        shortestJobProcess = currentReadyProcess;
                    }
                    currentReadyProcess = currentReadyProcess->nextInReadyQueue;
                }// End of iterating through to find the shortest time remaining

                // Needs to deal with removing the shortest job from the ready list (from front, back, and middle)
                struct Process* readiedProcess;

                if (readyHead->processID == shortestJobProcess->processID)
                {
                    // Dequeueing from the head of ready (normal dequeue)
                    readiedProcess = dequeueReadyProcess();
                }
                else if (readyTail->processID == shortestJobProcess->processID)
                {
                    // Dequeues from the tail of ready list
                    struct Process* currentReadyIterationProcess = readyHead;
                    while (currentReadyIterationProcess->nextInReadyQueue->nextInReadyQueue != NULL)
                    {currentReadyIterationProcess = currentReadyIterationProcess->nextInReadyQueue;}

                    readyTail = currentReadyIterationProcess;

                    readiedProcess = currentReadyIterationProcess->nextInReadyQueue;
                    currentReadyIterationProcess->nextInReadyQueue = NULL;
                    readiedProcess->nextInReadyQueue = NULL;
                    --readyProcessQueueSize;
                }
                else
                {
                    // Dequeues from the middle of ready list
                    struct Process* currentReadyIterationProcess = readyHead;

                    // Iterates until right before the middle block
                    while (currentReadyIterationProcess->nextInReadyQueue->processID != shortestJobProcess->processID)
                    {currentReadyIterationProcess = currentReadyIterationProcess->nextInReadyQueue;}

                    readiedProcess = currentReadyIterationProcess->nextInReadyQueue;

                    currentReadyIterationProcess->nextInReadyQueue =
                            currentReadyIterationProcess->nextInReadyQueue->nextInReadyQueue;
                    readiedProcess->nextInReadyQueue = NULL;
                    --readyProcessQueueSize;
                }

                // At this point, we have the shortest job process that should be sent, so sets it running
                readiedProcess->status = 2;
                readiedProcess->isFirstTimeRunning = true;

                char str[20];
                u_int32_t unsignedRandomInteger = (u_int32_t) atoi(fgets(str, 20, randomFile));
                // Prints out the random number, assuming the random flag is passed in
                if ((IS_RANDOM_MODE) && (currentPassNumber % 2 == 0))
                    printf("Find burst when choosing ready process to run %i\n", unsignedRandomInteger);

                u_int32_t newCPUBurst = 1 + (unsignedRandomInteger % shortestJobProcess->B);
                // Checks if the new CPU Burst time is greater than the time remaining
                if (newCPUBurst > (readiedProcess->C - readiedProcess->currentCPUTimeRun))  
                    newCPUBurst = readiedProcess->C - readiedProcess->currentCPUTimeRun;
                readiedProcess->CPUBurst = newCPUBurst;
                CURRENT_RUNNING_PROCESS = readiedProcess;
            }//::
            else if( schedulerAlgorithm == 5 )
            {
                // HRRN (w / s + 1) 
                u_int32_t i = 0;
                struct Process* currentReadyProcess = readyHead;
                struct Process* shortestJobProcess = readyHead;
                // PRINT CURRENTREADYPROCESS DETAILS
                
                double ans;
                for (; i < readyProcessQueueSize; ++i) // minimum sodhe che ---> mn = max(mn,i-j);
                {
                    double w1 = (1.0 * (shortestJobProcess->currentWaitingTime))/(shortestJobProcess->C - shortestJobProcess->currentCPUTimeRun); //temp w/s
                    double w2 = (1.0 * (currentReadyProcess->currentWaitingTime))/(currentReadyProcess->C - currentReadyProcess->currentCPUTimeRun);
                    printf("::::\nThese are the 'W / S' %f with PID = %i\n",w2,currentReadyProcess->processID);   
                    if ((w1) < (w2))
                    {
                        // Old shortest job is greater than the new one, sets the shortest job to point to the new one
                        ans=w2;
                        shortestJobProcess = currentReadyProcess;
                    }
                    currentReadyProcess = currentReadyProcess->nextInReadyQueue;
                }
                // End of iterating through to find the shortest time remaining
                //printf("::%i::%f::\n",shortestJobProcess->processID,ans);
                // Needs to deal with removing the shortest job from the ready list (from front, back, and middle)
                struct Process* readiedProcess;

                if (readyHead->processID == shortestJobProcess->processID)
                {
                    // Dequeueing from the head of ready (normal dequeue)
                    readiedProcess = dequeueReadyProcess();
                }
                else if (readyTail->processID == shortestJobProcess->processID)
                {
                    // Dequeues from the tail of ready list
                    struct Process* currentReadyIterationProcess = readyHead;
                    while (currentReadyIterationProcess->nextInReadyQueue->nextInReadyQueue != NULL)
                    {currentReadyIterationProcess = currentReadyIterationProcess->nextInReadyQueue;}

                    readyTail = currentReadyIterationProcess;

                    readiedProcess = currentReadyIterationProcess->nextInReadyQueue;
                    currentReadyIterationProcess->nextInReadyQueue = NULL;
                    readiedProcess->nextInReadyQueue = NULL;
                    --readyProcessQueueSize;
                }
                else
                {
                    // Dequeues from the middle of ready list
                    struct Process* currentReadyIterationProcess = readyHead;

                    // Iterates until right before the middle block
                    while (currentReadyIterationProcess->nextInReadyQueue->processID != shortestJobProcess->processID)
                    {currentReadyIterationProcess = currentReadyIterationProcess->nextInReadyQueue;}

                    readiedProcess = currentReadyIterationProcess->nextInReadyQueue;

                    currentReadyIterationProcess->nextInReadyQueue =
                            currentReadyIterationProcess->nextInReadyQueue->nextInReadyQueue;
                    readiedProcess->nextInReadyQueue = NULL;
                    --readyProcessQueueSize;
                }

                // At this point, we have the shortest job process that should be sent, so sets it running
                readiedProcess->status = 2;
                readiedProcess->isFirstTimeRunning = true;

                char str[20];
                u_int32_t unsignedRandomInteger = (u_int32_t) atoi(fgets(str, 20, randomFile));
                // Prints out the random number, assuming the random flag is passed in
                if ((IS_RANDOM_MODE) && (currentPassNumber % 2 == 0))
                    printf("Find burst when choosing ready process to run %i\n", unsignedRandomInteger);

                u_int32_t newCPUBurst = 1 + (unsignedRandomInteger % shortestJobProcess->B);
                // Checks if the new CPU Burst time is greater than the time remaining
                if (newCPUBurst > (readiedProcess->C - readiedProcess->currentCPUTimeRun))  
                    newCPUBurst = readiedProcess->C - readiedProcess->currentCPUTimeRun;
                readiedProcess->CPUBurst = newCPUBurst;
                CURRENT_RUNNING_PROCESS = readiedProcess;
            }
            else if( schedulerAlgorithm == 6 )
            {
                //srtn
                u_int32_t i = 0;
                struct Process* currentReadyProcess = readyHead;
                struct Process* shortestJobProcess = readyHead;
                for (; i < readyProcessQueueSize; ++i) // minimum sodhe che ---> mn = min(mn,i-j);
                {
                    if ((shortestJobProcess->C - shortestJobProcess->currentCPUTimeRun) <
                            (currentReadyProcess->C - currentReadyProcess->currentCPUTimeRun))
                    {
                        // Old shortest job is greater than the new one, sets the shortest job to point to the new one
                        shortestJobProcess = currentReadyProcess;
                    }
                    currentReadyProcess = currentReadyProcess->nextInReadyQueue;
                }// End of iterating through to find the shortest time remaining

                // Needs to deal with removing the shortest job from the ready list (from front, back, and middle)
                struct Process* readiedProcess;

                if (readyHead->processID == shortestJobProcess->processID)
                {
                    // Dequeueing from the head of ready (normal dequeue)
                    readiedProcess = dequeueReadyProcess();
                }
                else if (readyTail->processID == shortestJobProcess->processID)
                {
                    // Dequeues from the tail of ready list
                    struct Process* currentReadyIterationProcess = readyHead;
                    while (currentReadyIterationProcess->nextInReadyQueue->nextInReadyQueue != NULL)
                    {currentReadyIterationProcess = currentReadyIterationProcess->nextInReadyQueue;}

                    readyTail = currentReadyIterationProcess;

                    readiedProcess = currentReadyIterationProcess->nextInReadyQueue;
                    currentReadyIterationProcess->nextInReadyQueue = NULL;
                    readiedProcess->nextInReadyQueue = NULL;
                    --readyProcessQueueSize;
                }
                else
                {
                    // Dequeues from the middle of ready list
                    struct Process* currentReadyIterationProcess = readyHead;

                    // Iterates until right before the middle block
                    while (currentReadyIterationProcess->nextInReadyQueue->processID != shortestJobProcess->processID)
                    {currentReadyIterationProcess = currentReadyIterationProcess->nextInReadyQueue;}

                    readiedProcess = currentReadyIterationProcess->nextInReadyQueue;

                    currentReadyIterationProcess->nextInReadyQueue =
                            currentReadyIterationProcess->nextInReadyQueue->nextInReadyQueue;
                    readiedProcess->nextInReadyQueue = NULL;
                    --readyProcessQueueSize;
                }

                // At this point, we have the shortest job process that should be sent, so sets it running
                char str[20];
                u_int32_t unsignedRandomInteger = (u_int32_t) atoi(fgets(str, 20, randomFile));
                // Prints out the random number, assuming the random flag is passed in
                if ((IS_RANDOM_MODE) && (currentPassNumber % 2 == 0))
                    printf("Find burst when choosing ready process to run %i\n", unsignedRandomInteger);

                u_int32_t newCPUBurst = 1 + (unsignedRandomInteger % shortestJobProcess->B);
                // Checks if the new CPU Burst time is greater than the time remaining
                if (newCPUBurst > (readiedProcess->C - readiedProcess->currentCPUTimeRun))  
                    newCPUBurst = readiedProcess->C - readiedProcess->currentCPUTimeRun;
                readiedProcess->CPUBurst = newCPUBurst;
                if (readiedProcess->CPUBurst > 0)
                {
                    // There are no running processes, and the CPU Burst is positive, so sets the process to run
                    readiedProcess->status = 2;
                    readiedProcess->isFirstTimeRunning = true;

                    if (schedulerAlgorithm == 6)
                    {
                        // Scheduler is round robin, sets the quantum
                        readiedProcess->quantum = 2;
                    }
                    CURRENT_RUNNING_PROCESS = readiedProcess;
                }

                
               // CURRENT_RUNNING_PROCESS = readiedProcess;
            }
            else
            {
                // Is running one of the other schedulers, with no process currently running
                struct Process* readiedNode = dequeueReadyProcess();

                // Calculates CPU Burst stuff
                char str[20];
                u_int32_t unsignedRandomInteger = (u_int32_t) atoi(fgets(str, 20, randomFile));
                // Prints out the random number, assuming the random flag is passed in
                if ((IS_RANDOM_MODE) && (currentPassNumber % 2 == 0))
                    printf("Find burst when choosing ready process to run %i\n", unsignedRandomInteger);

                u_int32_t newCPUBurst = 1 + (unsignedRandomInteger % readiedNode->B);
                // Checks if the new CPU Burst time is greater than the time remaining
                if (newCPUBurst > (readiedNode->C - readiedNode->currentCPUTimeRun))
                    newCPUBurst = readiedNode->C - readiedNode->currentCPUTimeRun;
                readiedNode->CPUBurst = newCPUBurst;

                // Runs the process if the CPU burst is positive
                if (readiedNode->CPUBurst > 0)
                {
                    // There are no running processes, and the CPU Burst is positive, so sets the process to run
                    readiedNode->status = 2;
                    readiedNode->isFirstTimeRunning = true;

                    if (schedulerAlgorithm == 1)
                    {
                        // Scheduler is round robin, sets the quantum
                        readiedNode->quantum = 2;
                    }
                    CURRENT_RUNNING_PROCESS = readiedNode;
                }
            } // End of running FCFS, RR or Uniprogrammed scheduler process readying sequence
        } // End of dealing if there is no process running
    }// End of dealing with the ready queue

    // For uniprogrammed only
    if ((schedulerAlgorithm == 2) && (readyProcessQueueSize != 0))
    {
        // Things are still in the ready queue
        u_int32_t i = 0;
        for (; i < readyProcessQueueSize; ++i)
        {
            if (CURRENT_RUNNING_PROCESS != NULL)
            {
                // [UNIPROGRAMMED] There are running processes, suspends the ready process to the ready suspended pool
                struct Process* suspendedNode = dequeueReadyProcess();
                suspendedNode->status = 1;
                enqueueReadySuspendedProcess(suspendedNode);
            }
        }
    } // End of suspending to ready suspended any remaining processes [UNIPROGRAMMED]
} // End of the doReadyProcess function

/**
 * Alters all timers for any processes requiring a timer change
 * @param processContainer The original processes inputted, in array form
 * @param schedulerAlgorithm Which scheduler algorithm this function should run. 0 = FCFS, 1 = RR, 2 = UNI, 3 = SJF.
 */
void incrementTimers(struct Process processContainer[], u_int8_t schedulerAlgorithm)
{
    // Iterates through all processes, and alters any timers that need changing (decrementing CPUBurst if running, etc)
    u_int32_t i = 0;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        switch (processContainer[i].status)
        {
            case 0:
                // Node has not started
                break;
            case 3:
                // Node is I/O blocked (I/O time)
                ++processContainer[i].currentIOBlockedTime;
                --processContainer[i].IOBurst;
                break;
            case 2:
                // Node is running (CPU time)
                ++processContainer[i].currentCPUTimeRun;
                --processContainer[i].CPUBurst;
                if (schedulerAlgorithm == 1)
                {
                    // Process is utilising RR, and is running, so decrements quantum
                    --processContainer[i].quantum;
                }
                break;
            case 1:
                // Node is ready, or in ready suspended (waiting)
                ++processContainer[i].currentWaitingTime;
                break;
            case 4:
                // Node is terminated
                break;
            default:
                // Invalid node status, exiting now
                fprintf(stderr, "Error: Invalid process status code, exiting now!\n");
                exit(1);
        } // End of the per process status print statement
    }

    // Checks if a process has been blocked this cycle, used in calculating the overall time blocked
    for (i = 0; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        if (processContainer[i].status == 3)
        {
            // At least 1 process is blocked, increments the total count
            ++TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED;
            break;
        }
    }
} // End of the increment timers function

/************************ END OF RUNNING PROGRAM FUNCTIONS *************************************/

/**
 * Sets global flags for output depending on user input
 * @param argc The number of arguments in argv, where each argument is space deliminated
 * @param argv The command used to run the program, with each argument space deliminated
 */
u_int8_t setFlags(int32_t argc, char *argv[])
{
    if (argc == 2)
        return 1;
    else
    {
        if ((strcmp(argv[1], "--verbose") == 0) || (strcmp(argv[2], "--verbose") == 0))
            IS_VERBOSE_MODE = true;
        if ((strcmp(argv[1], "--random") == 0) || (strcmp(argv[2], "--random") == 0))
            IS_RANDOM_MODE = true;
        if ((strcmp(argv[1], "--random") != 0) && (strcmp(argv[1], "--verbose") != 0))
            return 1;
        else if ((strcmp(argv[2], "--random") != 0) && (strcmp(argv[2], "--verbose") != 0))
            return 2;
        else
            return 3;
    }
} // End of the setFlags function

/********************* START OF GLOBAL OUTPUT FUNCTIONS *********************************************************/

/**
 * Prints to standard output the original input
 * @param processContainer The original processes inputted, in array form
 */
void printStart(struct Process processContainer[])
{
    printf("The original input was: %i", TOTAL_CREATED_PROCESSES);

    u_int32_t i = 0;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", processContainer[i].A, processContainer[i].B,
               processContainer[i].C, processContainer[i].M);
    }
    printf("\n");
} // End of the print start function

/**
 * Prints to standard output the final output
 * @param finishedProcessContainer The terminated processes, in array form, and in the order they each finished in.
 */
void printFinal(struct Process finishedProcessContainer[])
{
    printf("The (sorted) input is: %i", TOTAL_CREATED_PROCESSES);

    u_int32_t i = 0;
    for (; i < TOTAL_FINISHED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", finishedProcessContainer[i].A, finishedProcessContainer[i].B,
               finishedProcessContainer[i].C, finishedProcessContainer[i].M);
    }
    printf("\n");
} // End of the print final function

/**
 * Prints out specifics for each process.
 * @param processContainer The original processes inputted, in array form
 */
void printProcessSpecifics(struct Process processContainer[])
{
    u_int32_t i = 0;
    printf("\n");
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf("Process %i:\n", processContainer[i].processID);
        printf("\t(A,B,C,M) = (%i,%i,%i,%i)\n", processContainer[i].A, processContainer[i].B,
               processContainer[i].C, processContainer[i].M);
        printf("\tFinishing time: %i\n", processContainer[i].finishingTime);
        printf("\tTurnaround time: %i\n", processContainer[i].finishingTime - processContainer[i].A);
        printf("\tI/O time: %i\n", processContainer[i].currentIOBlockedTime);
        printf("\tWaiting time: %i\n", processContainer[i].currentWaitingTime);
        printf("\n");
    }
} // End of the print process specifics function

/**
 * Prints out the summary data
 * @param processContainer The original processes inputted, in array form
 */
void printSummaryData(struct Process processContainer[])
{
    u_int32_t i = 0;
    double totalAmountOfTimeUtilisingCPU = 0.0;
    double totalAmountOfTimeIOBlocked = 0.0;
    double totalAmountOfTimeSpentWaiting = 0.0;
    double totalTurnaroundTime = 0.0;
    u_int32_t finalFinishingTime = CURRENT_CYCLE - 1;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        totalAmountOfTimeUtilisingCPU += processContainer[i].currentCPUTimeRun;
        totalAmountOfTimeIOBlocked += processContainer[i].currentIOBlockedTime;
        totalAmountOfTimeSpentWaiting += processContainer[i].currentWaitingTime;
        totalTurnaroundTime += (processContainer[i].finishingTime - processContainer[i].A);
    }

    // Calculates the CPU utilisation
    double CPUUtilisation = totalAmountOfTimeUtilisingCPU / finalFinishingTime;

    // Calculates the IO utilisation
    double IOUtilisation = (double) TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED / finalFinishingTime;

    // Calculates the throughput (Number of processes over the final finishing time times 100)
    double throughput =  100 * ((double) TOTAL_CREATED_PROCESSES/ finalFinishingTime);

    // Calculates the average turnaround time
    double averageTurnaroundTime = totalTurnaroundTime / TOTAL_CREATED_PROCESSES;

    // Calculates the average waiting time
    double averageWaitingTime = totalAmountOfTimeSpentWaiting / TOTAL_CREATED_PROCESSES;

    printf("Summary Data:\n");
    printf("\tFinishing time: %i\n", CURRENT_CYCLE - 1);
    printf("\tCPU Utilisation: %6f\n", CPUUtilisation);
    printf("\tI/O Utilisation: %6f\n", IOUtilisation);
    printf("\tThroughput: %6f processes per hundred cycles\n", throughput);
    printf("\tAverage turnaround time: %6f\n", averageTurnaroundTime);
    printf("\tAverage waiting time: %6f\n", averageWaitingTime);
} // End of the print summary data function

/**
 * Completely resets every counter, global variable, timer, and anything else that was changed since the start of a run
 * @param processContainer The original processes inputted, in array form
 */
void resetAfterRun(struct Process processContainer[])
{
    CURRENT_CYCLE = 0;
    TOTAL_STARTED_PROCESSES = 0;
    TOTAL_FINISHED_PROCESSES = 0;
    TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0;
    IS_FIRST_TIME_RUNNING_UNIPROGRAMMED = true;
    UNIPROGRAMMED_PROCESS = NULL;

    // readyQueue head & tail pointers
    readyHead = NULL;
    readyTail = NULL;
    readyProcessQueueSize = 0;

    // readySuspendedQueue head & tail pointers
    readySuspendedHead = NULL;
    readySuspendedTail = NULL;
    readySuspendedProcessQueueSize = 0;

    // blockedQueue head & tail pointers
    blockedHead = NULL;
    blockedTail = NULL;
    blockedProcessListSize = 0;

    CURRENT_RUNNING_PROCESS = NULL;

    u_int32_t i = 0;
    FILE* randomNumberFile = fopen(RANDOM_NUMBER_FILE_NAME, "r");
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        processContainer[i].status = 0;
        processContainer[i].nextInReadyQueue = NULL;
        processContainer[i].nextInReadySuspendedQueue = NULL;
        processContainer[i].nextInBlockedList = NULL;

        processContainer[i].finishingTime = -1;

        processContainer[i].currentCPUTimeRun = 0;
        processContainer[i].currentIOBlockedTime = 0;
        processContainer[i].currentWaitingTime = 0;

        processContainer[i].isFirstTimeRunning = false;

        processContainer[i].CPUBurst = randomOS(processContainer[i].B, randomNumberFile);
        processContainer[i].IOBurst = processContainer[i].M * processContainer[i].CPUBurst;

    }
    fclose(randomNumberFile);
} // End of reset after run function

/********************* END OF GLOBAL OUTPUT FUNCTIONS *********************************************************/

/**
 * Simulates the scheduler for each algorithm defined below.
 * @param currentPassNumber The current pass this simulation is run (either pass 1 or 2 for each algorithm)
 * @param processContainer The original processes inputted, in array form
 * @param finishedProcessContainer The terminated processes array, in which processes are added as they terminated
 * @param randomFile The file to retrieve a "random" number to be used in calculating CPU Burst
 * @param algorithmScheduler Which scheduler algorithm this function should run. 0 = FCFS, 1 = RR, 2 = UNI, 3 = SJF, 4=LJF.
 */
void simulateScheduler(u_int8_t currentPassNumber, struct Process processContainer[],
                       struct Process finishedProcessContainer[], FILE* randomFile, u_int8_t algorithmScheduler)
{
    if ((IS_VERBOSE_MODE) && ((currentPassNumber) % 2 == 0))
    {
        // Prints out the state of each process during the current cycle
        printf("Before cycle\t%i:\t", CURRENT_CYCLE);
        int i = 0;
        for (; i < TOTAL_CREATED_PROCESSES; ++i)
        {
            switch (processContainer[i].status)
            {
                case 0:
                    // Node has not started
                    printf("unstarted \t0\t");
                    break;
                case 1:
                    // Node is ready
                    printf("ready   \t0\t");
                    break;
                case 2:
                    // Node is running
                    printf("running \t%i\t", processContainer[i].CPUBurst + 1);
                    break;
                case 3:
                    // Node is I/O blocked
                    printf("blocked \t%i\t", processContainer[i].IOBurst + 1);
                    break;
                case 4:
                    // Node is terminated
                    printf("terminated \t0\t");
                    break;
                default:
                    // Invalid node status, exiting now
                    fprintf(stderr, "Error: Invalid process status code, exiting now!\n");
                    exit(1);
            } // End of the per process status print statement
        } // End of the per line for loop
        printf("\n");
    }

    doRunningProcesses(finishedProcessContainer, algorithmScheduler);
    doBlockedProcesses();

    if (TOTAL_STARTED_PROCESSES != TOTAL_CREATED_PROCESSES)
    {
        // Not all processes created, goes into creation loop
        createProcesses(processContainer);
    }

    // Checks whether the processes are all created, so it can skip creation if not required
    doReadyProcesses(algorithmScheduler, currentPassNumber, randomFile);
    incrementTimers(processContainer, algorithmScheduler);

    ++CURRENT_CYCLE;
} // End of the simulate round robin function

/****************************** END OF THE SIMULATION FUNCTIONS **************************************/

/******************* START OF THE OUTPUT WRAPPER FOR EACH SCHEDULING ALGORITHM *********************************/

/**
 * Scheduler wrapper for all scheduler types. NOTE: In order to keep the same format as the given outputs,
 * this scheduler runs each scheduler algorithm, twice, in order to be able to print out the final output early on.
 */
void schedulerWrapper (struct Process processContainer[], u_int8_t algorithmScheduler)
{
    // Prints the initial delimiter for each scheduler
    switch (algorithmScheduler)
    {
        case 0:
            printf("######################### START OF FIRST COME FIRST SERVE #########################\n");
            break;
        case 1:
            printf("######################### START OF ROUND ROBIN #########################\n");
            break;
        case 2:
            printf("######################### START OF UNIPROGRAMMED #########################\n");
            break;
        case 3:
            printf("######################### START OF SHORTEST JOB FIRST #########################\n");
            break;
        case 4://::
            printf("######################### START OF LONGEST JOB FIRST ##########################\n");
            break;
        case 5:
            printf("######################### HIGHEST RESPONSE RATION NEXT ##########################\n");
            break;
        case 6:
            printf("######################### SHORTEST REMAINING TIME NEXT ##########################\n");
            break;
        default:
            printf("Error: invalid scheduler algorithm utilised, defaulting to FCFS\n");
            algorithmScheduler = 0;
            break;
    }

    printStart(processContainer);
    u_int8_t currentPassNumber = 1;

    struct Process finishedProcessContainer[TOTAL_CREATED_PROCESSES];
    FILE* randomNumberFile = fopen(RANDOM_NUMBER_FILE_NAME, "r");
    // Runs this the first time in order to have the final output be available
    while (TOTAL_FINISHED_PROCESSES != TOTAL_CREATED_PROCESSES) //for every process....
        simulateScheduler(currentPassNumber, processContainer, finishedProcessContainer, randomNumberFile, algorithmScheduler);
    fclose(randomNumberFile);

    printFinal(finishedProcessContainer);
    resetAfterRun(processContainer);
    printf("\n");

    ++currentPassNumber;

    if (IS_VERBOSE_MODE)
        printf("This detailed printout gives the state and remaining burst for each process\n");

    randomNumberFile = fopen(RANDOM_NUMBER_FILE_NAME, "r");
    while (TOTAL_FINISHED_PROCESSES != TOTAL_CREATED_PROCESSES)
        simulateScheduler(currentPassNumber, processContainer, finishedProcessContainer, randomNumberFile, algorithmScheduler);
    fclose(randomNumberFile);

    // Prints which scheduling algorithm was used
    switch (algorithmScheduler)
    {
        case 0:
            printf("The scheduling algorithm used was First Come First Serve\n");
            break;
        case 1:
            printf("The scheduling algorithm used was Round Robin\n");
            break;
        case 2:
            printf("The scheduling algorithm used was Uniprogrammed\n");
            break;
        case 3:
            printf("The scheduling algorithm used was Shortest Job First\n");
            break;
        case 4:
            printf("The scheduling algorithm used was Longest Job First\n");
            break;
        case 5:
            printf("The scheduling algorithm used was Highest Response Ratio Next");
            break;
        case 6:
            printf("The scheduling algorithm used was Shortest Remaining Next");
            break;
        default:
            break;
    }

    printProcessSpecifics(processContainer);
    printSummaryData(processContainer);

    resetAfterRun(processContainer);        // Resets all values to initial conditions
    // Prints the final delimiter for each scheduler
    switch (algorithmScheduler)
    {
        case 0:
            printf("######################### END OF FIRST COME FIRST SERVE #########################\n");
            break;
        case 1:
            printf("######################### END OF ROUND ROBIN #########################\n");
            break;
        case 2:
            printf("######################### END OF UNIPROGRAMMED #########################\n");
            break;
        case 3:
            printf("######################### END OF SHORTEST JOB FIRST #########################\n");
            break;
        case 4:
            printf("######################### END OF LONGEST JOB FIRST #########################\n");
            break;
        case 5:
            printf("######################### END OF Highest Response Next ##########################\n");
            break;
        case 6:
            printf("######################### END OF SHORTEST REMAINING NEXT ##########################\n");
            break;
        default:
            break;
    }
} // End of the scheduler wrapper function for all schedule algorithms

/******************* END OF THE OUTPUT WRAPPER FOR EACH SCHEDULING ALGORITHM *********************************/

/**
 * Runs the actual process scheduler, based upon the commandline input. For example run commands, please see the README
 */
int main(int argc, char *argv[])
{
    // Reads in from file
    FILE* inputFile;
    FILE* randomNumberFile;
    char* filePath;

    filePath = argv[setFlags(argc, argv)]; // Sets any global flags from input
    inputFile = fopen(filePath, "r");

    // [ERROR CHECKING]: INVALID FILENAME
    if (inputFile == NULL) {
        fprintf(stderr, "Error: cannot open input file %s!\n",filePath);
        exit(1);
    }

    u_int32_t totalNumberOfProcessesToCreate;                    // Given as the first number in the mix
    fscanf(inputFile, "%i", &totalNumberOfProcessesToCreate);   // Reads in the indicator number for the mix

    randomNumberFile = fopen(RANDOM_NUMBER_FILE_NAME, "r");
    struct Process processContainer[totalNumberOfProcessesToCreate]; // Creates a container for all processes {datatype arr[3]}

    // Reads through the input, and creates all processes given, saving into an array
    u_int32_t currentNumberOfMixesCreated = 0;  // for loop no i=0
    for (; currentNumberOfMixesCreated < totalNumberOfProcessesToCreate; ++currentNumberOfMixesCreated)// for(i=0,i<3)
    {
        u_int32_t currentInputA;
        u_int32_t currentInputB;
        u_int32_t currentInputC;
        u_int32_t currentInputM;

        // Ain't C cool, that you can read in something like this that scans in the job
        fscanf(inputFile, " %*c%i %i %i %i%*c", &currentInputA, &currentInputB, &currentInputC, &currentInputM);

        processContainer[currentNumberOfMixesCreated].A = currentInputA; //arr[i].F = x
        processContainer[currentNumberOfMixesCreated].B = currentInputB;
        processContainer[currentNumberOfMixesCreated].C = currentInputC;
        processContainer[currentNumberOfMixesCreated].M = currentInputM;

        processContainer[currentNumberOfMixesCreated].processID = currentNumberOfMixesCreated;
        processContainer[currentNumberOfMixesCreated].status = 0; //not started
        processContainer[currentNumberOfMixesCreated].finishingTime = -1; //not defined or inf

        processContainer[currentNumberOfMixesCreated].currentCPUTimeRun = 0;
        processContainer[currentNumberOfMixesCreated].currentIOBlockedTime = 0;
        processContainer[currentNumberOfMixesCreated].currentWaitingTime = 0;

        processContainer[currentNumberOfMixesCreated].CPUBurst = randomOS(processContainer[currentNumberOfMixesCreated].B, randomNumberFile); // cpu burst = 1..B 
        processContainer[currentNumberOfMixesCreated].IOBurst = processContainer[currentNumberOfMixesCreated].M * processContainer[currentNumberOfMixesCreated].CPUBurst;

        processContainer[currentNumberOfMixesCreated].isFirstTimeRunning = false; //pata nai kyu

        processContainer[currentNumberOfMixesCreated].quantum = 2; // Value provided as described in requirements

        processContainer[currentNumberOfMixesCreated].nextInBlockedList = NULL;
        processContainer[currentNumberOfMixesCreated].nextInReadyQueue = NULL;
        processContainer[currentNumberOfMixesCreated].nextInReadySuspendedQueue = NULL;
        ++TOTAL_CREATED_PROCESSES;
    }
    // All processes from mix instantiated
    fclose(inputFile);
    fclose(randomNumberFile);

    // First Come First Serve Run
    schedulerWrapper(processContainer, 0);

    // Round Robin Run
    schedulerWrapper(processContainer, 1);

    // Uniprogrammed Run
    schedulerWrapper(processContainer, 2);

    // Shortest Job First Run
    schedulerWrapper(processContainer, 3);

    //:: Longest Job Next
    schedulerWrapper(processContainer, 4);

    //::HRRN
    schedulerWrapper(processContainer, 5);

    //::Shortest Remainning Time Next
    schedulerWrapper(processContainer, 6);
    /*
        schedulerW(4,5,6) add karsu
        HRRN
        SRTN
        priority
        LRJN 
    */

    return EXIT_SUCCESS;
} // End of the main function
