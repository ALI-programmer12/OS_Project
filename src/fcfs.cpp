#include "../include/fcfs.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
using namespace std;

static void sortArrival(vector<Process>& list) {
    sort(list.begin(), list.end(), [](Process a, Process b) {
        return a.arrivalTime < b.arrivalTime;
    });
}

void runFCFS(vector<Process>& list) {
    sortArrival(list);

    int time = 0;

    for (auto &p : list) {
        if (time < p.arrivalTime)
            time = p.arrivalTime;

        p.startTime = time;
        p.finishTime = time + p.burstTime;
        p.turnaroundTime = p.finishTime - p.arrivalTime;
        p.waitingTime = p.turnaroundTime - p.burstTime;

        time = p.finishTime;
    }

    cout << "\nFCFS Completed\n";
}
