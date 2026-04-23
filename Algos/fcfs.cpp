#include "fcfs.h"
#include <algorithm>
using namespace std;

void firstComeFirstServe(Process p[], int n) {
    // sort by arrival time
    for (int i = 0; i < n-1; i++)
        for (int j = i+1; j < n; j++)
            if (p[j].at < p[i].at) { Process tmp=p[i]; p[i]=p[j]; p[j]=tmp; }

    int time = 0;
    for (int i = 0; i < n; i++) {
        if (time < p[i].at) time = p[i].at;
        time += p[i].bt;
        p[i].tat = time - p[i].at;
        p[i].wt  = p[i].tat - p[i].bt;
    }
}