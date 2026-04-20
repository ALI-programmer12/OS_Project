#include "Preemptive_Priorityscheduling.h"
#include <climits>
using namespace std;

void preemptivePriorityScheduling(Process p[], int n) {
    for (int i = 0; i < n; i++) { p[i].rt = p[i].bt; p[i].wt = 0; p[i].tat = 0; }

    int completed = 0, time = 0;
    while (completed < n) {
        int idx = -1, best = INT_MAX;
        for (int i = 0; i < n; i++)
            if (p[i].at <= time && p[i].rt > 0 && p[i].pr < best) { best = p[i].pr; idx = i; }
        if (idx == -1) { time++; continue; }
        p[idx].rt--;
        time++;
        if (p[idx].rt == 0) {
            p[idx].tat = time - p[idx].at;
            p[idx].wt  = p[idx].tat - p[idx].bt;
            completed++;
        }
    }
}
