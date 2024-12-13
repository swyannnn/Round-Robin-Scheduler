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
    int pid;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int startTime;
    int completionTime;
    int waitingTime;
    int turnaroundTime;
    int responseTime;
    bool hasExecuted;
    Status status;
    int ioRemainingTime;
    int priority;
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

bool enqueueQ(Queue *q, int pid) {
    if (isFull(q)) {
        printf("Ready Queue is full. Cannot enqueue Process P%d.\n", pid);
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

    printf("\nEnter arrival times, burst times, and priorities for your processes:\n");
    for (int i = 0; i < *n; i++) {
        printf("Process %d:\n", i + 1);
        int arrival, burst, prio;
        while (!readInt("  Arrival Time: ", &arrival, 0, 9999, false)) {}
        while (!readInt("  Burst Time: ", &burst, 1, 9999, false)) {}
        while (!readInt("  Priority: ", &prio, 1, 9999, false)) {}

        processes[i].pid = i + 1;
        processes[i].arrivalTime = arrival;
        processes[i].burstTime = burst;
        processes[i].remainingTime = burst;
        processes[i].status = READY;
        processes[i].hasExecuted = false;
        processes[i].ioRemainingTime = *globalIOWait;
        processes[i].priority = prio;
        processes[i].waitingTime = 0;
        processes[i].turnaroundTime = 0;
        processes[i].responseTime = 0;
        processes[i].startTime = 0;
        processes[i].completionTime = 0;
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

    printf("%-8d", ganttChart[0].startTime);
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
        printf("%d\tIdle\t\tIdle\t\t-\n", currentTime);
    } else {
        if (remainingTime >= 0)
            printf("%-8d P%-7d %-16s %-8d\n", currentTime, pid, status, remainingTime);
        else
            printf("%d\tP%d\t\t%s\t\t-\n", currentTime, pid, status);
    }
}

void printProcessTable(Process processes[], int n) {
    printf("\n%-8s %-8s %-8s %-10s %-12s %-12s %-8s %-8s\n",
           "Process", "Arrival", "Burst", "Priority", "Completion", "Turnaround", "Waiting", "Response");
    for (int i = 0; i < n; i++) {
        printf("P%-7d %-8d %-8d %-10d %-12d %-12d %-8d %-8d\n",
               processes[i].pid,
               processes[i].arrivalTime,
               processes[i].burstTime,
               processes[i].priority,
               processes[i].completionTime,
               processes[i].turnaroundTime,
               processes[i].waitingTime,
               processes[i].responseTime);
    }
}

void printMetrics(double avgTurnaround, double avgWaiting, double avgResponse, double cpuUtil) {
    printf("\nRound Robin Scheduling Performance:\n");
    printf("Average Turnaround Time: %.2f\n", avgTurnaround);
    printf("Average Waiting Time: %.2f\n", avgWaiting);
    printf("Average Response Time: %.2f\n", avgResponse);
    printf("Total CPU Utilization: %.2f%%\n", cpuUtil);
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
            processes[i].ioRemainingTime--;
            if (processes[i].ioRemainingTime <= 0) {
                processes[i].status = READY;
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
            enqueueQ(readyQueue, processes[i].pid);
            printStatus(currentTime, processes[i].pid, "Ready", processes[i].remainingTime);
        }
    }
}

//------------------------------//
//    Main Scheduling Logic     //
//------------------------------//

void runRoundRobin(Process processes[], int n, int timeQuantum, int globalIOWait) {
    Queue readyQueue;
    initQueue(&readyQueue);

    // Dynamically allocated GanttChart for safety if simulation runs long
    GanttChartEntry *ganttChart = malloc(sizeof(GanttChartEntry) * 10000);
    int ganttCount = 0;

    int currentTime = 0;
    int completed = 0;
    int lastPID = IDLE;
    int totalBusyTime = 0;

    printHeader();

    // Continue until all processes are completed
    while (completed < n) {
        // Check for new arrivals
        handleNewArrivals(processes, n, currentTime, &readyQueue);
        // Update blocked processes
        updateBlockedProcesses(processes, n, currentTime, globalIOWait, &readyQueue);

        if (isEmpty(&readyQueue)) {
            // CPU is idle
            printStatus(currentTime, IDLE, "Idle", -1);
            if (lastPID != IDLE) {
                if (ganttCount > 0) ganttChart[ganttCount - 1].endTime = currentTime;
                ganttChart[ganttCount].pid = IDLE;
                ganttChart[ganttCount].startTime = currentTime;
                ganttCount++;
            }
            currentTime++;
            lastPID = IDLE;
            continue;
        }

        int currentPID = dequeueQ(&readyQueue);
        Process *currentProcess = getProcessByPID(processes, n, currentPID);
        if (!currentProcess) {
            printf("Error: Process P%d not found.\n", currentPID);
            free(ganttChart);
            return;
        }

        // If process arrives in the future, CPU idle
        if (currentProcess->arrivalTime > currentTime) {
            enqueueQ(&readyQueue, currentPID);
            printStatus(currentTime, IDLE, "Idle", -1);
            if (lastPID != IDLE) {
                if (ganttCount > 0) ganttChart[ganttCount - 1].endTime = currentTime;
                ganttChart[ganttCount].pid = IDLE;
                ganttChart[ganttCount].startTime = currentTime;
                ganttCount++;
            }
            currentTime++;
            lastPID = IDLE;
            continue;
        }

        // Mark the process as running if it was ready
        if (currentProcess->status == READY) {
            currentProcess->status = RUNNING;
            if (!currentProcess->hasExecuted) {
                currentProcess->startTime = currentTime;
                currentProcess->responseTime = currentTime - currentProcess->arrivalTime;
                currentProcess->hasExecuted = true;
            }
        }

        // Gantt Chart update
        if (lastPID != currentPID) {
            if (lastPID != IDLE && ganttCount > 0) {
                ganttChart[ganttCount - 1].endTime = currentTime;
            }
            ganttChart[ganttCount].pid = currentPID;
            ganttChart[ganttCount].startTime = currentTime;
            ganttCount++;
        }

        int execTime = (currentProcess->remainingTime < timeQuantum) ? currentProcess->remainingTime : timeQuantum;

        for (int t = 0; t < execTime; t++) {
            printStatus(currentTime, currentPID, "Running", currentProcess->remainingTime);
            currentTime++;
            totalBusyTime++;
            currentProcess->remainingTime--;

            // Update waiting time for all READY processes
            for (int i = 0; i < n; i++) {
                if (processes[i].status == READY && processes[i].pid != currentPID) {
                    processes[i].waitingTime++;
                }
            }

            // Check new arrivals during execution
            handleNewArrivals(processes, n, currentTime, &readyQueue);
            // Update blocked processes
            updateBlockedProcesses(processes, n, currentTime, globalIOWait, &readyQueue);

            // If process completes
            if (currentProcess->remainingTime == 0) {
                currentProcess->status = COMPLETED;
                currentProcess->completionTime = currentTime;
                currentProcess->turnaroundTime = currentProcess->completionTime - currentProcess->arrivalTime;
                currentProcess->waitingTime = currentProcess->turnaroundTime - currentProcess->burstTime;
                completed++;
                printStatus(currentTime, currentPID, "Completed", 0);
                break;
            }
        }

        // If not completed and quantum expired
        if (currentProcess->remainingTime > 0 && currentProcess->status != COMPLETED) {
            if (globalIOWait > 0) {
                currentProcess->status = BLOCKED;
                currentProcess->ioRemainingTime = globalIOWait;
                printStatus(currentTime, currentPID, "Blocked", currentProcess->remainingTime);
            } else {
                currentProcess->status = READY;
                enqueueQ(&readyQueue, currentProcess->pid);
                printStatus(currentTime, currentPID, "Ready", currentProcess->remainingTime);
            }
        }

        if (ganttCount > 0) {
            ganttChart[ganttCount - 1].endTime = currentTime;
        }

        lastPID = (currentProcess->status == RUNNING) ? currentPID : IDLE;
    }

    // Calculate metrics
    double avgTurnaround, avgWaiting, avgResponse;
    calculateMetrics(processes, n, &avgTurnaround, &avgWaiting, &avgResponse);
    double cpuUtil = calculateCPUUtilization(totalBusyTime, (completed > 0) ? processes[0].completionTime : 0);
    // Note: The above CPU utilization calculation might need a re-check if processes are completed at different times.
    // Alternatively, we can use currentTime as total simulation time:
    cpuUtil = calculateCPUUtilization(totalBusyTime, (int)(processes[0].completionTime + totalBusyTime));

    // Print results
    printGanttChart(ganttChart, ganttCount);
    printProcessTable(processes, n);
    printMetrics(avgTurnaround, avgWaiting, avgResponse, cpuUtil);

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
