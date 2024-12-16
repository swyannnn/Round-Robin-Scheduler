#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

//------------------------------//
//         Configuration        //
//------------------------------//

#define MAX_PROCESSES 10
#define IDLE -1

//------------------------------//
//      Data Structures         //
//------------------------------//

// Process Status Enumeration
typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    COMPLETED
} Status;

// Structure to represent each process
typedef struct {
    int pid;                // Process ID
    int arrivalTime;        // Arrival Time
    int burstTime;          // Total CPU Burst Time
    int remainingTime;      // Remaining CPU Burst Time
    int startTime;          // Time when process first gets the CPU
    int completionTime;     // Time when process completes
    int waitingTime;        // Total waiting time in ready queue
    int turnaroundTime;     // Completion Time - Arrival Time
    int responseTime;       // Start Time - Arrival Time
    bool hasExecuted;       // Flag to check if the process has been executed at least once
    bool hasBlocked;        // Flag to check if the process has been blocked once
    Status status;          // Current status of the process
    int ioRemainingTime;    // Remaining I/O Wait Time when blocked
    int blockedAtTime;      // Time when process was blocked
} Process;

// Structure for Gantt Chart Entry
typedef struct {
    int pid;
    int startTime;
    int endTime;
} GanttChartEntry;

// Circular Queue structure for Ready Processes
typedef struct {
    int items[MAX_PROCESSES];
    int front;
    int rear;
} Queue;

//------------------------------//
//       Queue Operations       //
//------------------------------//

void initQueue(Queue *q) {
    q->front = -1;
    q->rear = -1;
}

bool isEmpty(Queue *q) {
    return q->front == -1;
}

bool isFull(Queue *q) {
    return ((q->rear + 1) % MAX_PROCESSES) == q->front;
}

bool isInQueue(Queue *q, int pid) {
    if (isEmpty(q)) {
        return false;
    }
    int i = q->front;
    while (1) {
        if (q->items[i] == pid) {
            return true;
        }
        if (i == q->rear) {
            break;
        }
        i = (i + 1) % MAX_PROCESSES;
    }
    return false;
}

bool enqueueQ(Queue *q, int pid) {
    if (isFull(q)) {
        return false;
    }
    if (isInQueue(q, pid)) {
        return false;
    }
    if (isEmpty(q)) {
        q->front = 0;
    }
    q->rear = (q->rear + 1) % MAX_PROCESSES;
    q->items[q->rear] = pid;
    return true;
}

int dequeueQ(Queue *q) {
    if (isEmpty(q)) {
        return -1;
    }
    int pid = q->items[q->front];
    if (q->front == q->rear) {
        q->front = q->rear = -1;
    } else {
        q->front = (q->front + 1) % MAX_PROCESSES;
    }
    return pid;
}

//------------------------------//
//         Input Handling        //
//------------------------------//

bool readInt(const char *prompt, int *value, int minVal, int maxVal, bool allowMax) {
    int temp;
    printf("%s", prompt);
    if (scanf("%d", &temp) != 1) {
        printf("Invalid input. Please enter an integer.\n");
        while (getchar() != '\n');
        return false;
    }
    if ((allowMax && (temp < minVal || temp > maxVal)) || (!allowMax && temp < minVal)) {
        if (allowMax) {
            printf("Invalid input. Value must be between %d and %d.\n", minVal, maxVal);
        } else {
            printf("Invalid input. Value must be >= %d.\n", minVal);
        }
        while (getchar() != '\n');
        return false;
    }
    *value = temp;
    return true;
}

bool getUserInput(Process processes[], int *n, int *timeQuantum, int *globalIOWait) {
    // Number of processes
    while (!readInt("Enter the number of processes (1-10): ", n, 1, MAX_PROCESSES, true)) {}
    // Time quantum
    while (!readInt("Enter the Time Quantum (TQ): ", timeQuantum, 1, 9999, false)) {}
    // I/O Wait Time
    while (!readInt("Enter I/O Wait Time: ", globalIOWait, 0, 9999, true)) {}

    printf("\nEnter arrival times and burst times for your processes:\n");
    for (int i = 0; i < *n; i++) {
        printf("Process %d:\n", i + 1);
        int arrival, burst;
        while (!readInt("  Arrival Time: ", &arrival, 0, 9999, false)) {}
        while (!readInt("  Burst Time: ", &burst, 1, 9999, false)) {}

        processes[i].pid = i + 1;
        processes[i].arrivalTime = arrival;
        processes[i].burstTime = burst;
        processes[i].remainingTime = burst;
        processes[i].status = READY;
        processes[i].hasExecuted = false;
        processes[i].hasBlocked = false; 
        processes[i].ioRemainingTime = *globalIOWait;
        processes[i].waitingTime = 0;
        processes[i].turnaroundTime = 0;
        processes[i].responseTime = 0;
        processes[i].startTime = 0;
        processes[i].completionTime = 0;
        processes[i].blockedAtTime = -1; // Initially no blocking time here, inside the loop
    }

    return true;
}

//------------------------------//
//       Metrics Calculation     //
//------------------------------//

void calculateMetrics(Process processes[], int n, double *avgTurnaround, double *avgWaiting, double *avgResponse) {
    double totalTurnaround = 0, totalWaiting = 0, totalResponse = 0;
    for (int i = 0; i < n; i++) {
        totalTurnaround += processes[i].turnaroundTime;
        totalWaiting += processes[i].waitingTime;
        totalResponse += processes[i].responseTime;
    }
    *avgTurnaround = totalTurnaround / n;
    *avgWaiting = totalWaiting / n;
    *avgResponse = totalResponse / n;
}

double calculateCPUUtilization(int totalBusyTime, int totalTime) {
    if (totalTime == 0) return 0.0;
    return ((double)totalBusyTime / totalTime) * 100;
}

//------------------------------//
//          Printing             //
//------------------------------//

void printGanttChart(GanttChartEntry ganttChart[], int ganttCount) {
    if (ganttCount == 0) {
        printf("\nNo Gantt Chart data available.\n");
        return;
    }

    printf("\nGantt Chart:\n");
    for (int i = 0; i < ganttCount; i++) {
        if (ganttChart[i].pid == IDLE) {
            printf("| %-5s ", "Idle");
        } else {
            printf("| P%-4d ", ganttChart[i].pid);
        }
    }
    printf("|\n");

    printf("%-7d ", ganttChart[0].startTime);
    for (int i = 0; i < ganttCount; i++) {
        printf("%-8d", ganttChart[i].endTime);
    }
    printf("\n");
}

void printHeader() {
    printf("\nTime\tProcess ID\tStatus\t\tRemaining Time\n");
}

void printStatus(int currentTime, int pid, const char* status, int remainingTime) {
    if (pid == IDLE) {
        printf("%-8d %-14s %-16s %-8s\n", currentTime, "Idle", "Idle", "-");
    } else {
        printf("%-8d P%-13d %-16s ", currentTime, pid, status);
        if (remainingTime >= 0) {
            printf("%-8d\n", remainingTime);
        } else {
            printf("%-8s\n", "-");
        }
    }
}

void printProcessTable(Process processes[], int n) {
    printf("\n%-8s %-8s %-8s %-12s %-12s %-8s %-8s\n",
           "Process", "Arrival", "Burst", "Completion", "Turnaround", "Waiting", "Response");
    for (int i = 0; i < n; i++) {
        printf("P%-7d %-8d %-8d %-12d %-12d %-8d %-8d\n",
               processes[i].pid,
               processes[i].arrivalTime,
               processes[i].burstTime,
               processes[i].completionTime,
               processes[i].turnaroundTime,
               processes[i].waitingTime,
               processes[i].responseTime);
    }
}

void printMetrics(double avgTurnaround, double avgWaiting, double avgResponse, double cpuUtil, int contextSwitches) {
    printf("\nRound Robin Scheduling Performance:\n");
    printf("Average Turnaround Time: %.2f\n", avgTurnaround);
    printf("Average Waiting Time: %.2f\n", avgWaiting);
    printf("Average Response Time: %.2f\n", avgResponse);
    printf("Total CPU Utilization: %.2f%%\n", cpuUtil);
    printf("Total Context Switches: %d\n", contextSwitches); // Added line to print context switches
}

//------------------------------//
//       Helper Functions        //
//------------------------------//

Process* getProcessByPID(Process processes[], int n, int pid) {
    for (int i = 0; i < n; i++) {
        if (processes[i].pid == pid) return &processes[i];
    }
    return NULL;
}

void updateBlockedProcesses(Process processes[], int n, int currentTime, int globalIOWait, Queue *readyQueue) {
    for (int i = 0; i < n; i++) {
        if (processes[i].status == BLOCKED) {
            // Check if enough time has passed since blocking
            if (currentTime >= processes[i].blockedAtTime + globalIOWait) {
                processes[i].status = READY;
                processes[i].blockedAtTime = -1; // Reset blocking time
                // we want multiple I/O cycles, so we reset ioRemainingTime here
                enqueueQ(readyQueue, processes[i].pid);
                printStatus(currentTime, processes[i].pid, "Ready", processes[i].remainingTime);
            }
        }
    }
}


void handleNewArrivals(Process processes[], int n, int currentTime, Queue *readyQueue) {
    for (int i = 0; i < n; i++) {
        // Note: To avoid repeatedly enqueuing a process, we check if it hasn't executed yet.
        // If the process is ready and arrives at the current time (and hasn't been enqueued before),
        // we enqueue it now.
        if (processes[i].arrivalTime == currentTime && processes[i].status == READY && !processes[i].hasExecuted) {
            bool enqueued = enqueueQ(readyQueue, processes[i].pid);
            if (enqueued) {
                printStatus(currentTime, processes[i].pid, "Ready", processes[i].remainingTime);
            }
        }
    }
}

//------------------------------//
//    Main Scheduling Logic     //
//------------------------------//

void runRoundRobin(Process processes[], int n, int timeQuantum, int globalIOWait) {
    Queue readyQueue;
    initQueue(&readyQueue);

    // Allocate a large enough Gantt chart array
    GanttChartEntry *ganttChart = malloc(sizeof(GanttChartEntry) * 10000);
    int ganttCount = 0;

    int currentTime = 0;
    int completed = 0;
    int totalBusyTime = 0;

    // Track the last PID that was running on the CPU
    // Start with CPU idle
    int lastPID = IDLE;

    // Initialize Context Switch Counter
    int contextSwitches = 0;
    printHeader();

    while (completed < n) {
        // Handle new arrivals and blocked processes
        handleNewArrivals(processes, n, currentTime, &readyQueue);
        updateBlockedProcesses(processes, n, currentTime, globalIOWait, &readyQueue);

        // Check if we have any ready process
        if (isEmpty(&readyQueue)) {
            // No process is ready, CPU remains idle
            if (lastPID != IDLE) {
                // Close the last running process block
                if (ganttCount > 0 && ganttChart[ganttCount - 1].endTime == 0) {
                    ganttChart[ganttCount - 1].endTime = currentTime;
                }
                lastPID = IDLE;
                contextSwitches++;
            } else {
                // If the last block was idle, no need to start a new one
                if (ganttCount > 0 && ganttChart[ganttCount - 1].pid == IDLE) {
                    currentTime++;
                    continue;
                }
            }
            // **Start a new idle block**
            ganttChart[ganttCount].pid = IDLE;
            ganttChart[ganttCount].startTime = currentTime;
            ganttCount++;
            currentTime++;
            continue;
        }

        // There is a process ready to run
        int currentPID = dequeueQ(&readyQueue);
        Process *currentProcess = getProcessByPID(processes, n, currentPID);

        // Context switch handling for Gantt chart
        if (lastPID != currentPID) {
            if (currentTime > 0){
                contextSwitches++;
            }
            // Close the previous block if it hasn't been closed
            if (ganttCount > 0 && ganttChart[ganttCount - 1].endTime == 0) {
                ganttChart[ganttCount - 1].endTime = currentTime;
            }

            // Start a new block for this process
            ganttChart[ganttCount].pid = currentPID;
            ganttChart[ganttCount].startTime = currentTime;
            ganttCount++;
        }

        // Update process status if it was READY
        if (currentProcess->status == READY) {
            currentProcess->status = RUNNING;
            if (!currentProcess->hasExecuted) {
                currentProcess->startTime = currentTime;
                currentProcess->responseTime = currentTime - currentProcess->arrivalTime;
                currentProcess->hasExecuted = true;
            }
        }

        // Determine how long to run the process (up to the time quantum or remaining time)
        int execTime = (currentProcess->remainingTime < timeQuantum) ? currentProcess->remainingTime : timeQuantum;

        bool processCompleted = false;
        for (int t = 0; t < execTime; t++) {
            printStatus(currentTime, currentPID, "Running", currentProcess->remainingTime);
            currentTime++;
            totalBusyTime++;
            currentProcess->remainingTime--;

            // After incrementing time, handle arrivals and I/O
            handleNewArrivals(processes, n, currentTime, &readyQueue);
            updateBlockedProcesses(processes, n, currentTime, globalIOWait, &readyQueue);

            // Update waiting time for all other READY processes
            for (int i = 0; i < n; i++) {
                if (processes[i].status == READY && processes[i].pid != currentPID) {
                    processes[i].waitingTime++;
                }
            }

            // Check if process completes now
            if (currentProcess->remainingTime == 0) {
                currentProcess->status = COMPLETED;
                currentProcess->completionTime = currentTime;
                currentProcess->turnaroundTime = currentProcess->completionTime - currentProcess->arrivalTime;
                currentProcess->waitingTime = currentProcess->turnaroundTime - currentProcess->burstTime;
                completed++;
                printStatus(currentTime, currentPID, "Completed", 0);

                // Close this process's block in Gantt chart
                if (ganttCount > 0 && ganttChart[ganttCount - 1].pid == currentPID && ganttChart[ganttCount - 1].endTime == 0) {
                    ganttChart[ganttCount - 1].endTime = currentTime;
                }

                lastPID = IDLE; // CPU will be idle next iteration since no immediate process is running now
                processCompleted = true;
                break;
            }
        }

        if (processCompleted) {
            lastPID = currentPID; // Set to the current process's PID
            processCompleted = true;
            continue;
        }

        // Process did not complete after quantum
        if (currentProcess->remainingTime > 0 && currentProcess->status != COMPLETED) {
            // Needs I/O?
            if (globalIOWait > 0 && !currentProcess->hasBlocked) {
                currentProcess->status = BLOCKED;
                currentProcess->blockedAtTime = currentTime;
                currentProcess->hasBlocked = true;
                printStatus(currentTime, currentPID, "Blocked", currentProcess->remainingTime);
            } 
            else {
                // No I/O wait, re-queue the process
                currentProcess->status = READY;
                enqueueQ(&readyQueue, currentProcess->pid);
                printStatus(currentTime, currentPID, "Ready", currentProcess->remainingTime);
            }
            lastPID = currentPID;
            // Close current process block in Gantt chart
            if (ganttCount > 0 && ganttChart[ganttCount - 1].pid == currentPID && ganttChart[ganttCount - 1].endTime == 0) {
                ganttChart[ganttCount - 1].endTime = currentTime;
            }
        }
    }

    // If the last block isn't ended (e.g., ended in Idle), close it
    if (ganttCount > 0 && ganttChart[ganttCount - 1].endTime == 0) {
        ganttChart[ganttCount - 1].endTime = currentTime;
    }

    // Calculate metrics
    double avgTurnaround, avgWaiting, avgResponse;
    calculateMetrics(processes, n, &avgTurnaround, &avgWaiting, &avgResponse);

    // CPU Utilization calculation
    // Note: total time is from 0 to currentTime; totalBusyTime is how long CPU was actually running processes
    double cpuUtil = calculateCPUUtilization(totalBusyTime, currentTime);

    // Print results
    printGanttChart(ganttChart, ganttCount);
    printProcessTable(processes, n);
    printMetrics(avgTurnaround, avgWaiting, avgResponse, cpuUtil, contextSwitches);

    free(ganttChart);
}

//------------------------------//
//            main()            //
//------------------------------//

int main() {
    Process processes[MAX_PROCESSES];
    int n, timeQuantum, globalIOWait;

    if (!getUserInput(processes, &n, &timeQuantum, &globalIOWait)) {
        printf("Error reading user input.\n");
        return 1;
    }
    runRoundRobin(processes, n, timeQuantum, globalIOWait);
    return 0;
}