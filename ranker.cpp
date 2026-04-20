#include "ranker.h"
#include <iostream>
#include <algorithm>
using namespace std;

void rankAlgorithms() {
    cout << "\n============================================================\n";
    cout << "                     ALGORITHM RANKING\n";
    cout << "============================================================\n";
    cout << "Rank | Algorithm                    | Avg WT | Avg TAT | Score\n";
    cout << "-----+------------------------------+--------+---------+------\n";

    vector<pair<float, int>> score;
    for (int i = 0; i < (int)results.size(); i++)
        score.push_back({results[i].avgWT + results[i].avgTAT, i});

    sort(score.begin(), score.end());

    for (int i = 0; i < (int)score.size(); i++) {
        auto &r = results[score[i].second];
        printf(" %-3d | %-28s | %6.2f | %7.2f | %.2f\n",
               i+1, r.name.c_str(), r.avgWT, r.avgTAT, score[i].first);
    }
    cout << "\n  BEST ALGORITHM: " << results[score[0].second].name << "\n";
    cout << "============================================================\n";
}
