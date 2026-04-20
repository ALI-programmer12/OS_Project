#ifndef PROCESS_H
#define PROCESS_H

struct Process {
    int pid;
    int arrivalTime;
    int burstTime;

    int startTime;
    int finishTime;
    int waitingTime;
    int turnaroundTime;

    int priority;
    int remainingTime;

    bool completed;
    bool inQueue;
};

struct GanttBlock {
    int pid;
    int start;
    int end;
};

#endif
