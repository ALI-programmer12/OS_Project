#include "LRTF.h"
#include "simulator.h"
using namespace std;

void lrtfScheduling(Process p[], int n) {
    for (int i = 0; i < n; i++) { p[i].rt = p[i].bt; p[i].wt = 0; p[i].tat = 0; }

    int completed = 0, time = 0;
    while (completed < n) {
        int idx = -1, maxRT = -1;
        for (int i = 0; i < n; i++)
            if (p[i].at <= time && p[i].rt > 0 && p[i].rt > maxRT) { maxRT = p[i].rt; idx = i; }
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
