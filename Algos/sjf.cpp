#include "sjf.h"
#include "simulator.h"
#include <climits>
using namespace std;

// Non-preemptive SJF
void shortestJobFirst(Process p[], int n) {
    for (int i = 0; i < n; i++) { p[i].rt = p[i].bt; p[i].wt = 0; p[i].tat = 0; }

    int time = 0, completed = 0;
    while (completed < n) {
        int idx = -1, minBT = INT_MAX;
        for (int i = 0; i < n; i++)
            if (p[i].at <= time && p[i].rt > 0 && p[i].bt < minBT) { minBT = p[i].bt; idx = i; }
        if (idx == -1) { time++; continue; }
        currentGantt.push_back(make_tuple(time, p[idx].pid, p[idx].bt));
        time += p[idx].bt;
        p[idx].tat = time - p[idx].at;
        p[idx].wt  = p[idx].tat - p[idx].bt;
        p[idx].rt  = 0;
        completed++;
    }
}

// Preemptive SJF (SRTF)
void shortestRemainingTime(Process p[], int n) {
    for (int i = 0; i < n; i++) { p[i].rt = p[i].bt; p[i].wt = 0; p[i].tat = 0; }

    int time = 0, completed = 0;
    while (completed < n) {
        int idx = -1, minRT = INT_MAX;
        for (int i = 0; i < n; i++)
            if (p[i].at <= time && p[i].rt > 0 && p[i].rt < minRT) { minRT = p[i].rt; idx = i; }
        if (idx == -1) { time++; continue; }
        currentGantt.push_back(make_tuple(time, p[idx].pid, 1));
        p[idx].rt--;
        time++;
        if (p[idx].rt == 0) {
            p[idx].tat = time - p[idx].at;
            p[idx].wt  = p[idx].tat - p[idx].bt;
            completed++;
        }
    }
}