#include <stdio.h>
#include <time.h>
#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

#include "Go.h"
#include "point.h"
#include "zobrist.h"
#include "omp.h"
#include "mpi.h"
#include "common.h"

Point Mcts_zobrist::run(Board* curr_board, int rank, uint64_t curr_time, int& num_games, ZobristHash z) {
	int bsize = curr_board->get_bsize();

    clock_gettime(CLOCK_REALTIME, &start);
    std::vector<uint64_t> search_path;

    Board* curr_board_copy = new Board(bsize, z);
    while(!checkAbort()) {
        curr_board_copy->copy_board(curr_board);
        run_iteration(curr_board_copy, curr_time, num_games);
    }
    delete curr_board_copy;

    std::vector<Point> legal_moves = curr_board->get_next_legal_moves();

    double maxv = -1.0;
    int best_id = -1;
    for(int i = 0; i < legal_moves.size(); i++){
        int id = Point::point_to_id(legal_moves[i], bsize);
        uint64_t child_hash = z.updateHash(curr_board->get_hash(), id, curr_board->ToPlay());

        TNode* child_data = table->getNode(child_hash);

        double v = child_data->sims;
        if(v > maxv){
            maxv = v;
            best_id = id;
        }
    }

    return Point::id_to_point(best_id, bsize);
}

void Mcts_zobrist::run_iteration(Board* curr_board, uint64_t curr_time, int& num_games) {
    // run iteration
}

uint64_t Mcts_zobrist::selection(Board* board, uint64_t curr_time) {
    // select a node
}
void Mcts_zobrist::expand(Board* board, uint64_t curr_time) {
    // expand the tree
}
void Mcts_zobrist::backprop(const std::vector<uint64_t>& search_path, double wins, double sims, uint64_t current_time) {
    // backprop
}

bool Mcts_zobrist::checkAbort() {
    // check abort (can we factor this out?)
}