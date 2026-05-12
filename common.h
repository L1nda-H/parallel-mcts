#ifndef COMMON_H
#define COMMON_H

#include <iostream>

#define C 1.4
#define EPSILON 10e-64
#define MAX_STEP 100 // redefine to avoid repeat game
#define BILLION 1000000000L
#define MILLION 1000000.0
#define VIRTUAL_LOSS 0.5
#define TT_REMOTE_BATCH_THRESHOLD 512
#define TT_SHARE_DEPTH 1

#define DEBUGMODE true

enum COLOR {
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2,
    OUT = 3
};

#define MAX_POINTS 450 
#define TRANSPOSITION_TABLE_SIZE 32000000
// #define N_SIMS_SHARE 500 // Min num of simulations that has passed a node before it must be shared

inline void log_msg(std::string message, int rank) {
    if (rank == 0 && DEBUGMODE){
        std::cerr << message << "\n";
        fflush(stderr);
    }
}

#endif
