#include "simulator.h"
#include "ranker.h"
#include <iostream>
#include <iomanip>
using namespace std;

// Forward declarations — all use Process[]
void firstComeFirstServe(Process[], int);
void roundRobin(Process[], int, int);
void shortestJobFirst(Process[], int);
void shortestRemainingTime(Process[], int);
void lrtfScheduling(Process[], int);
void mlfqScheduling(Process[], int);
void preemptivePriorityScheduling(Process[], int);
void nonPreemptivePriorityScheduling(Process[], int);

vector<Result> results;

static float avgWT(Process p[], int n) {
    float s=0; for(int i=0;i<n;i++) s+=p[i].wt; return s/n;
}
static float avgTAT(Process p[], int n) {
    float s=0; for(int i=0;i<n;i++) s+=p[i].tat; return s/n;
}
static void reset(Process p[], int n) {
    for(int i=0;i<n;i++) { p[i].wt=0; p[i].tat=0; p[i].rt=p[i].bt; }
}

void simulate(Process p[], int n, string name, int algo, int quantum) {
    reset(p, n);

    switch(algo) {
        case 1: firstComeFirstServe(p, n);             break;
        case 2: roundRobin(p, n, quantum);             break;
        case 3: shortestJobFirst(p, n);                break;
        case 4: shortestRemainingTime(p, n);           break;
        case 5: lrtfScheduling(p, n);                  break;
        case 6: mlfqScheduling(p, n);                  break;
        case 7: preemptivePriorityScheduling(p, n);    break;
        case 8: nonPreemptivePriorityScheduling(p, n); break;
        default: cout<<"Invalid algo\n"; return;
    }

    cout << "\n------------------------------------------------------------\n";
    cout << " " << name << "\n";
    cout << "------------------------------------------------------------\n";
    cout << left << setw(6) << "PID"
         << setw(12) << "Arrival"
         << setw(12) << "Burst"
         << setw(12) << "Wait"
         << setw(12) << "Turnaround" << "\n";
    for(int i=0;i<n;i++)
        cout << left << setw(6) << p[i].pid
             << setw(12) << p[i].at
             << setw(12) << p[i].bt
             << setw(12) << p[i].wt
             << setw(12) << p[i].tat << "\n";

    float w = avgWT(p,n), t = avgTAT(p,n);
    cout << "\n  Avg Waiting Time   : " << fixed << setprecision(2) << w;
    cout << "\n  Avg Turnaround Time: " << t << "\n";

    results.push_back({name, w, t});
}
