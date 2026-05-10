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

void simulate(Board* b, double* wins, double* sims) {
    COLOR player = b->ToPlay();
    int curr_step = 0;
    thread_local std::mt19937 rng(std::random_device{}());
	while (curr_step < MAX_STEP) {
		std::vector<Point> moves = b->get_next_legal_moves();
		if (moves.size() == 0) {
			break;
		}

		std::uniform_int_distribution<int> dist(0, moves.size() - 1);
		b->update_board(moves[dist(rng)]); 

		curr_step++;
	}

	int score = b->quick_score();
	if ((score > 0 && player == BLACK) || (score < 0 && player == WHITE)) {
		*wins += 1.0;
	}
	*sims += 1.0;
}

void Mcts_zobrist::backprop(const std::vector<uint64_t>& search_path, double wins, double sims, uint64_t curr_time) {
    // backprop
    for (uint64_t hash : search_path) {
        table->updateNode(hash, wins, sims, curr_time);
    }
}

void Mcts_zobrist::run_iteration(Board* curr_board, uint64_t curr_time, int& num_games) {
    std::vector<uint64_t> search_path;

    uint64_t current_hash = curr_board->get_hash();
    double n = table->getNode(current_hash)->sims;
    ZobristHash z = curr_board->get_htable();
    search_path.push_back(current_hash);

    bool reached_unexpanded_node = false;

    while (curr_board->get_next_legal_moves().size() != 0 && !reached_unexpanded_node) {
        // do we really need to scramble?
        std::vector<Point> legal_moves = curr_board->get_next_legal_moves();
        uint64_t best_hash = 0;
        Point best_move(-1, -1);
        double maxv = -1.0;

        // expand step gets eliminated bc everything is hashed
        // this is selection
        TNode* root = table->getNode(current_hash);
        for (Point p : legal_moves) {
            int pid = Point::point_to_id(p, curr_board->get_bsize());
            uint64_t c_hash = z.updateHash(current_hash, pid, curr_board->ToPlay());
            TNode* c_node = table->getNode(c_hash);

            if (c_node->sims == 0) {
                best_move = p;
                best_hash = c_hash;
                reached_unexpanded_node = true;
                break;
            }

            double v = c_node->wins / (c_node->sims + EPSILON) + C * sqrt(log(n + EPSILON) / (c_node->sims + EPSILON));

            if (v > maxv) {
                maxv = v;
                best_move = p;
                best_hash = c_hash;
            }
        }

        curr_board->update_board(best_move);
        current_hash = best_hash;
        search_path.push_back(current_hash);

        double wins = 0.0;
        double sims = 0.0;
        simulate(curr_board, &wins, &sims);

        backprop(search_path, wins, sims, curr_time);
    }
}

bool Mcts_zobrist::checkAbort() {
    // check abort (can we factor this out?)
    bool current_abort;
	#pragma omp atomic read
	current_abort = abort;

	if (!current_abort) {
		u_int64_t diff;
		// each thread gets separate copy of current_end
		struct timespec current_end;
		clock_gettime(CLOCK_REALTIME, &current_end);
		diff = BILLION * (current_end.tv_sec - start.tv_sec) + current_end.tv_nsec - start.tv_nsec;

		if (diff / MILLION > maxTime) {
			#pragma omp atomic write
			abort = true;
			return true;
		}
		return false;
	}
	return true;
}