#ifndef SIMULATOR_H
#define SIMULATOR_H
#include "process.h"
#include "result.h"
#include <vector>
#include <string>
extern std::vector<Result> results;
void simulate(Process p[], int n, std::string name, int algo, int quantum = 4);
#endif
