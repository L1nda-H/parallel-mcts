#ifndef COMMON_H
#define COMMON_H

#define C 1.4
#define EPSILON 10e-64
#define MAX_STEP 100 // redefine to avoid repeat game
#define BILLION 1000000000L
#define MILLION 1000000.0
#define VIRTUAL_LOSS 0.5

enum COLOR {
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2,
    OUT = 3
};

#define MAX_POINTS 450 
#define TRANSPOSITION_TABLE_SIZE 32000000
// #define N_SIMS_SHARE 500 // Min num of simulations that has passed a tree node before it must be shared
// #define ALPHA 0.5

#endif