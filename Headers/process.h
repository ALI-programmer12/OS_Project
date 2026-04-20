#ifndef PROCESS_H
#define PROCESS_H

struct Process {
    int pid;
    int at;   // Arrival Time
    int bt;   // Burst Time
    int pr;   // Priority (lower = higher priority)
    int rt;   // Remaining Time
    int wt;   // Waiting Time
    int tat;  // Turnaround Time
};

#endif
