#include <stdio.h>
#include <time.h>
#include <random>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <unordered_set>
#include <numeric>
#include <vector>

#include "Go.h"
#include "point.h"
#include "zobrist.h"
#include "omp.h"
#include "mpi.h"

namespace {
void collectSharedHashes(Board* board, int depth, std::unordered_set<uint64_t>& hashes) {
    if (depth <= 0) {
        return;
    }

    std::vector<Point> moves = board->get_next_legal_moves();
    if (moves.empty()) {
        return;
    }

    int bsize = board->get_bsize();
    ZobristHash z = board->get_htable();
    for (Point move : moves) {
        Board child(bsize, z);
        child.copy_board(board);
        child.update_board(move);
        hashes.insert(child.get_hash());
        collectSharedHashes(&child, depth - 1, hashes);
    }
}
}

Point Mcts_zobrist::run(Board* curr_board, int rank, int& num_games) {
	int bsize = curr_board->get_bsize();
    player = curr_board->ToPlay();
    std::vector<Point> legal_moves = curr_board->get_next_legal_moves();
    if (legal_moves.empty()) {
        return Point::id_to_point(-1, bsize);
    }

    if (omp_get_max_threads() <= 1) {
        if (rank == 0) {
            std::cerr << "Requires at least 2 threads per node.\n";
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    std::unordered_set<uint64_t> shared_hashes;
    collectSharedHashes(curr_board, TT_SHARE_DEPTH, shared_hashes);
    table->startIoThread(shared_hashes);

    clock_gettime(CLOCK_REALTIME, &start);
    std::vector<uint64_t> search_path;

    ZobristHash z = curr_board->get_htable();
    int games = 0;
    int num_worker_tasks = omp_get_max_threads() - 1;

    #pragma omp parallel
    {
        #pragma omp single
        {
            #pragma omp task
            {
                table->runIoThread();
            }

            #pragma omp taskgroup
            {
                for (int worker = 0; worker < num_worker_tasks; worker++) {
                    #pragma omp task
                    {
                        Board* curr_board_copy = new Board(bsize, z);
                        Board* scratch_board = new Board(bsize, z);
                        int local_games = 0;
                        while(!checkAbort()) {
                            curr_board_copy->copy_board(curr_board);
                            run_iteration(curr_board_copy, scratch_board, rank, local_games);
                        }

                        #pragma omp atomic
                        games += local_games;
                        delete curr_board_copy;
                        delete scratch_board;
                    }
                }
            }

            table->stopIoThread();

            #pragma omp taskwait
        }
    }
    
    num_games += games;
    int best_id = -1;

    if (rank == 0) {
        Board* scratch_board = new Board(bsize, z);

        double maxv = -1.0;
        std::vector<int> best_ids;
        for (Point p : legal_moves) {
            scratch_board->copy_board(curr_board);
            int id = Point::point_to_id(p, bsize);
            scratch_board->update_board(p);
            uint64_t child_hash = scratch_board->get_hash();

            double child_wins = 0.0;
            double child_sims = 0.0;
            table->getStats(child_hash, &child_wins, &child_sims);

            if (child_sims > maxv) {
                maxv = child_sims;
                best_ids.clear();
                best_ids.push_back(id);
            } else if (child_sims == maxv) {
                best_ids.push_back(id);
            }
        }

        if (best_ids.empty()) {
            best_ids.push_back(Point::point_to_id(legal_moves[0], bsize));
        }

        thread_local std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, best_ids.size() - 1);
        best_id = best_ids[dist(gen)];

        delete scratch_board;
    }

    MPI_Bcast(&best_id, 1, MPI_INT, 0, MPI_COMM_WORLD);

    return Point::id_to_point(best_id, bsize);
}

void Mcts_zobrist::simulate(Board* b, double* wins, double* sims) {
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
	if (score > 0) {
		*wins += 1.0;
	}
	*sims += 1.0;
}

void Mcts_zobrist::backprop(const std::vector<uint64_t>& search_path, double wins, double sims) {
    for (uint64_t hash : search_path) {
        table->updateNode(hash, wins, sims - VIRTUAL_LOSS, move);
    }
}

void Mcts_zobrist::run_iteration(Board* curr_board, Board* scratch_board, int rank, int& num_games) {
    std::vector<uint64_t> search_path;
    std::unordered_set<uint64_t> seen_hashes;

    uint64_t current_hash = curr_board->get_hash();
    ZobristHash z = curr_board->get_htable();
    search_path.push_back(current_hash);
    seen_hashes.insert(current_hash);

    int consecutive_passes = 0;

    bool reached_unexpanded_node = false;

    std::vector<Point> legal_moves = curr_board->get_next_legal_moves();
    thread_local std::mt19937 gen(std::random_device{}());
    std::shuffle(legal_moves.begin(), legal_moves.end(), gen);
    // if (rank == 0 && omp_get_thread_num() == 0) {
    //     std::cerr << "selecting node\n";
    // }
    int selection_steps = 0;
    while (legal_moves.size() != 0 && !reached_unexpanded_node &&
           consecutive_passes < 2 && selection_steps < MAX_STEP) {
        Point best_move(-1, -1);
        double maxv = -1.0;
        COLOR player_to_move = curr_board->ToPlay();

        double parent_wins = 0.0;
        double parent_sims = 0.0;
        table->getStats(current_hash, &parent_wins, &parent_sims);

        for (Point p : legal_moves) {
            scratch_board->copy_board(curr_board);
            int pid = Point::point_to_id(p, curr_board->get_bsize());
            scratch_board->update_board(p);
            double child_wins = 0.0;
            double child_sims = 0.0;
            table->getStats(scratch_board->get_hash(), &child_wins, &child_sims);

            if (child_sims == 0) {
                best_move = p;
                reached_unexpanded_node = true;
                break;
            }

            double exploration = 0.0;
            if (parent_sims > 1.0) {
                exploration = C * sqrt(log(parent_sims) / (child_sims + EPSILON));
            }
            double black_win_rate = child_wins / (child_sims + EPSILON);
            double player_win_rate = player_to_move == BLACK ? black_win_rate : 1.0 - black_win_rate;
            double v = player_win_rate + exploration;

            if (v > maxv) {
                maxv = v;
                best_move = p;
            }
        }

        curr_board->update_board(best_move);
        current_hash = curr_board->get_hash();
        search_path.push_back(current_hash);
        selection_steps++;

        if (!seen_hashes.insert(current_hash).second) {
            break;
        }

        table->updateLocalVirtualLoss(current_hash, move);

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

    simulate(curr_board, &wins, &sims);
    num_games += sims;

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