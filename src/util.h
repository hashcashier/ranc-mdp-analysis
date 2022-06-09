//
// Created by rami on 18/03/2021.
//

#ifndef POSH_MDP_ANALYSIS_UTIL_H
#define POSH_MDP_ANALYSIS_UTIL_H

#include <unistd.h>
#include <ios>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>

typedef unsigned short int UI16;
typedef unsigned int UI32;
typedef unsigned long long UI64;
typedef long long SI64;
typedef double LD;

using namespace std;


void mem_usage();

template <typename T>
void printVector(vector<T> const &input, ostream &stream) {
    stream << "[ ";
    for (auto const &v : input) {
        stream << UI64(v) << ' ';
    }
    stream << ']';
}

template <typename T>
void printBm(T bm, UI16 bmLen, ostream &stream) {
    stream << "[ ";
    for (UI32 i = 0; i < bmLen; i++) {
        stream << (bm&1) << ' ';
        bm >>= 1;
    }
    stream << ']';
}

template <typename T>
T safeSum(vector<T> const &vec, size_t start = 0, size_t end = SIZE_MAX) {
    if (start >= vec.size()) {
        return 0;
    } else if (end >= vec.size()) {
        return accumulate(vec.begin()+start, vec.end(), 0);
    }
    return accumulate(vec.begin()+start, vec.begin()+end, 0);
}

inline UI16 countOnes(UI64 bitmask) {
    UI16 res = 0;
    while (bitmask > 0) {
        res += bitmask&1;
        bitmask >>= 1;
    }
    return res;
}

#endif //POSH_MDP_ANALYSIS_UTIL_H
