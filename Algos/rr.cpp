#include "rr.h"
#include <queue>
using namespace std;

void roundRobin(Process p[], int n, int tq) {
    for (int i = 0; i < n; i++) { p[i].rt = p[i].bt; p[i].wt = 0; p[i].tat = 0; }

    queue<int> q;
    vector<bool> inQueue(n, false);
    int time = 0, completed = 0;

    // sort by arrival time first
    for (int i = 0; i < n-1; i++)
        for (int j = i+1; j < n; j++)
            if (p[j].at < p[i].at) { Process tmp=p[i]; p[i]=p[j]; p[j]=tmp; }

    for (int i = 0; i < n; i++)
        if (p[i].at <= time) { q.push(i); inQueue[i] = true; }

    while (completed < n) {
        if (q.empty()) {
            time++;
            for (int i = 0; i < n; i++)
                if (!inQueue[i] && p[i].at <= time) { q.push(i); inQueue[i] = true; }
            continue;
        }
        int idx = q.front(); q.pop();
        int exec = (p[idx].rt < tq) ? p[idx].rt : tq;
        p[idx].rt -= exec;
        time += exec;

        for (int i = 0; i < n; i++)
            if (!inQueue[i] && p[i].at <= time) { q.push(i); inQueue[i] = true; }

        if (p[idx].rt == 0) {
            p[idx].tat = time - p[idx].at;
            p[idx].wt  = p[idx].tat - p[idx].bt;
            completed++;
        } else {
            q.push(idx);
        }
    }
}