#include "MLFQ.h"
#include "simulator.h"
#include <queue>
using namespace std;

// 3-level MLFQ: Q1(tq=4), Q2(tq=8), Q3(FCFS)
void mlfqScheduling(Process p[], int n) {
    for (int i = 0; i < n; i++) { p[i].rt = p[i].bt; p[i].wt = 0; p[i].tat = 0; }

    // sort by arrival
    for (int i = 0; i < n-1; i++)
        for (int j = i+1; j < n; j++)
            if (p[j].at < p[i].at) { Process tmp=p[i]; p[i]=p[j]; p[j]=tmp; }

    queue<int> q1, q2, q3;
    int level[100] = {0}; // track which queue each process is in (1,2,3)
    bool inQueue[100] = {false};

    int time = 0, completed = 0;

    auto enqueueArrivals = [&]() {
        for (int i = 0; i < n; i++)
            if (!inQueue[i] && p[i].at <= time && p[i].rt > 0) {
                q1.push(i); inQueue[i] = true; level[i] = 1;
            }
    };
    enqueueArrivals();

    while (completed < n) {
        if (q1.empty() && q2.empty() && q3.empty()) { time++; enqueueArrivals(); continue; }

        int idx, tq, lv;
        if (!q1.empty())      { idx = q1.front(); q1.pop(); tq = 4;  lv = 1; }
        else if (!q2.empty()) { idx = q2.front(); q2.pop(); tq = 8;  lv = 2; }
        else                  { idx = q3.front(); q3.pop(); tq = p[idx].rt; lv = 3; }

        int exec = (p[idx].rt < tq) ? p[idx].rt : tq;
        currentGantt.push_back(make_tuple(time, p[idx].pid, exec));
        p[idx].rt -= exec;
        time += exec;
        enqueueArrivals();

        if (p[idx].rt == 0) {
            p[idx].tat = time - p[idx].at;
            p[idx].wt  = p[idx].tat - p[idx].bt;
            completed++;
        } else {
            // demote
            if (lv == 1)      { q2.push(idx); level[idx] = 2; }
            else if (lv == 2) { q3.push(idx); level[idx] = 3; }
            else              { q3.push(idx); }
        }
    }
}
