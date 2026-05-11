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

Point Mcts_zobrist::run(Board* curr_board, int rank, int& num_games) {
	int bsize = curr_board->get_bsize();

    clock_gettime(CLOCK_REALTIME, &start);
    std::vector<uint64_t> search_path;

    ZobristHash z = curr_board->get_htable();
    int games = 0;

    #pragma omp parallel 
    {
        if (rank == 0 && omp_get_thread_num() == 0) std::cerr << "num threads: " << omp_get_num_threads() << "\n";

        Board* curr_board_copy = new Board(bsize, z);
        Board* scratch_board = new Board(bsize, z);
        int local_games = 0;
        while(!checkAbort()) {
            curr_board_copy->copy_board(curr_board);
            run_iteration(curr_board_copy, scratch_board, rank, num_games);
            local_games++;
        }
        delete curr_board_copy;

        #pragma omp atomic
        games += local_games;
    }
    
    num_games += games;

    std::vector<Point> legal_moves = curr_board->get_next_legal_moves();

    double maxv = -1.0;
    std::vector<int> best_ids;
    for(int i = 0; i < legal_moves.size(); i++){
        int id = Point::point_to_id(legal_moves[i], bsize);
        uint64_t child_hash = z.updateHash(curr_board->get_hash(), id, curr_board->ToPlay());

        TNode* child_data = table->getNode(child_hash);

        double v = child_data->sims;

        if (rank == 0 && omp_get_thread_num() == 0) {
            std::cerr << "Point " << Point::pt_to_gtp(legal_moves[i], bsize) << " had " << v << " sims\n";
        }

        if(v > maxv){
            maxv = v;
            best_ids.clear();
            best_ids.push_back(id);
        } else if (v == maxv) {
            best_ids.push_back(id);
        }
    }

    if (rank == 0 && omp_get_thread_num() == 0) std::cerr << "Num of best ids is " << best_ids.size() << "\n";
    thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, best_ids.size() - 1);
    int best_id = best_ids[dist(gen)];

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

void Mcts_zobrist::backprop(const std::vector<uint64_t>& search_path, double wins, double sims) {
    // backprop
    for (uint64_t hash : search_path) {
        // -1.0 to account for virtual loss
        table->updateNode(hash, wins, sims - 1.0, move);
    }
}

void Mcts_zobrist::run_iteration(Board* curr_board, Board* scratch_board, int rank, int& num_games) {
    std::vector<uint64_t> search_path;

    uint64_t current_hash = curr_board->get_hash();
    ZobristHash z = curr_board->get_htable();
    search_path.push_back(current_hash);

    int consecutive_passes = 0;

    bool reached_unexpanded_node = false;

    std::vector<Point> legal_moves = curr_board->get_next_legal_moves();
    thread_local std::mt19937 gen(std::random_device{}());
    std::shuffle(legal_moves.begin(), legal_moves.end(), gen);

    // if (rank == 0 && omp_get_thread_num() == 0) std::cerr << "selecting node \n";
    while (legal_moves.size() != 0 && !reached_unexpanded_node && consecutive_passes < 2) {
        Point best_move(-1, -1);
        double maxv = -1.0;

        TNode* parent_node = table->getNode(current_hash);
        double parent_sims = parent_node->sims;

        // if (rank == 0 && omp_get_thread_num() == 0) std::cerr << "iterating legal moves \n";
        for (Point p : legal_moves) {
            scratch_board->copy_board(curr_board);
            int pid = Point::point_to_id(p, curr_board->get_bsize());
            scratch_board->update_board(p);
            TNode* c_node = table->getNode(scratch_board->get_hash());

            if (c_node->sims == 0) {
                best_move = p;
                reached_unexpanded_node = true;
                break;
            }

            double v = c_node->wins / (c_node->sims + EPSILON) + C * sqrt(log(parent_sims + EPSILON) / (c_node->sims + EPSILON));

            if (v > maxv) {
                maxv = v;
                best_move = p;
            }
        }

        // if (rank == 0 && omp_get_thread_num() == 0) std::cerr << "selected hash " << best_hash << "\n";

        curr_board->update_board(best_move);
        current_hash = curr_board->get_hash();
        search_path.push_back(current_hash);

        
        // if (rank == 0 && omp_get_thread_num() == 0) {
        //     std::cerr << "point selected to simulate is " << Point::pt_to_gtp(best_move, curr_board->get_bsize()) << " \n";
        // }

        #pragma omp atomic
        table->getNode(current_hash)->sims += VIRTUAL_LOSS;

        if (best_move.i == -1) {
            consecutive_passes++;
        } else {
            consecutive_passes = 0;
        }

        if (!reached_unexpanded_node) {
            legal_moves = curr_board->get_next_legal_moves();
            thread_local std::mt19937 gen(std::random_device{}());
            std::shuffle(legal_moves.begin(), legal_moves.end(), gen);
        }
    }

    double wins = 0.0;
    double sims = 0.0;
    // if (rank == 0 && omp_get_thread_num() == 0) std::cerr << "Simulating \n";
    simulate(curr_board, &wins, &sims);

    // if (rank == 0 && omp_get_thread_num() == 0) std::cerr << "Backpropagating\n";
    backprop(search_path, wins, sims);
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