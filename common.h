#ifndef COMMON_H
#define COMMON_H

#include <iostream>

#define C 1.4
#define EPSILON 10e-64
#define MAX_STEP 100 // redefine to avoid repeat game
#define BILLION 1000000000L
#define MILLION 1000000.0
#define VIRTUAL_LOSS 0.5
#define TT_REMOTE_BATCH_THRESHOLD 1024
#define TT_SHARE_DEPTH 1

#define DEBUGMODE false

enum COLOR {
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2,
    OUT = 3
};

#define MAX_POINTS 450 
#define TRANSPOSITION_TABLE_SIZE 32000000

inline void log_msg(std::string message, int rank) {
    if (rank == 0 && DEBUGMODE){
        std::cerr << message << "\n";
        fflush(stderr);
    }
}

#endif
