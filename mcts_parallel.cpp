#include <stdio.h>
#include <time.h>
#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

#include "mcts.h"
#include "Go.h"
#include "point.h"
#include "omp.h"
#include "mpi.h"
#include "common.h"

static double atomic_read_double(double* value) {
	double result;
	#pragma omp atomic read
	result = *value;
	return result;
}

static void apply_virtual_loss(TreeNode* node) {
	#pragma omp atomic
	node->sims += VIRTUAL_LOSS;
}

Point Mcts::run(Board* curr_board, int rank, int& num_games) {
	// sync all MPI processes before starting
	MPI_Barrier(MPI_COMM_WORLD);
	
	int bsize = curr_board->get_bsize();

	clock_gettime(CLOCK_REALTIME, &start);
	#pragma omp parallel
	{
		Board* curr_board_copy = new Board(bsize);
		while(!checkAbort()) {
			curr_board_copy->copy_board(curr_board);
			run_iteration(root, curr_board_copy, num_games);
		}
		delete curr_board_copy;
	}
	int num_possible_moves = bsize * bsize;
	int pass_index = num_possible_moves;
	int num_moves_with_pass = num_possible_moves + 1;

	std::vector<double> local_sims(num_moves_with_pass, 0.0);

	std::vector<TreeNode*>* children = root->get_children();

	if (children != nullptr) {
		for (TreeNode* c : *children) {
			int id = Point::point_to_id(c->get_move(), bsize);
			int index = id > -1 ? id : pass_index;
			local_sims[index] = atomic_read_double(&c->sims);
		}
	}

	std::vector<double> global_sims(num_moves_with_pass, 0.0);

	MPI_Reduce(local_sims.data(), global_sims.data(), num_moves_with_pass, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

	int best_id = -1;
	
	if(rank == 0){
		double maxv = -1.0;
		for(int i = 0; i < num_moves_with_pass; i++){
			double v = global_sims[i];
			if(v > maxv){
				maxv = v;
				best_id = (i == pass_index) ? -1 : i;
			}
		}
	}

	MPI_Bcast(&best_id, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if(rank == 0){
		num_games += std::accumulate(global_sims.begin(), global_sims.end(), 0.0);
	}

	return Point::id_to_point(best_id, bsize);
}

TreeNode* Mcts::selection(TreeNode* node) {
	double maxv = -1.0;
	TreeNode* maxn = NULL;
	double n = atomic_read_double(&node->sims);

	if (n < 1.0) {
		n = 1.0;
	}

	std::vector<TreeNode*>* children = node->get_children();
	if (children == nullptr) return node;
	for (TreeNode* c : *children) {
		double child_sims = atomic_read_double(&c->sims);
		double child_wins = atomic_read_double(&c->wins);
		if (child_sims == 0.0) {
			return c;
		}
		double v = child_wins / (child_sims + EPSILON) + C * sqrt(log(n + EPSILON) / (child_sims + EPSILON));
		if (v > maxv) {
			maxv = v;
			maxn = c;
		}
	}
	return maxn;
}

void Mcts::expand(TreeNode* node, Board* board) {
	std::vector<Point> moves_vec = board->get_next_legal_moves();
	moves_vec.push_back(Point(-1, -1));
	
	// instead of adding to the tree, store the children locally
	std::vector<TreeNode*>* local_children = new std::vector<TreeNode*>();

	if (!moves_vec.empty()) {
		thread_local std::mt19937 rng(std::random_device{}());

		for (Point nxt_move : moves_vec) {
			TreeNode* child = new TreeNode(nxt_move);
			child->parent = node;
			local_children->push_back(child);
		}
	}

	// if compare and swap was unsuccessful, remove the local children
	if (!node->try_set_children(local_children)) {
		for (TreeNode* c : *local_children) {
			delete c;
		}
		delete local_children;
	}
}

void Mcts::backprop(TreeNode* node, int win_increase, int sim_increase) {
	bool lv = false;
	while (node != NULL && node->parent != NULL) {
		#pragma omp atomic
		node->sims += (sim_increase - VIRTUAL_LOSS);

		if (lv) {
			#pragma omp atomic
			node->wins += win_increase;
		}

		node = node->parent;
		lv = !lv;
	}

	if (node->parent != NULL) {
		
		node = node->parent;
	}
}

void run_simulation(Board* b, double* wins, double* sims) {
	COLOR player = b->ToPlay();
	int curr_step = 0;
	int consecutive_passes = 0;
	thread_local std::mt19937 rng(std::random_device{}());
	while (curr_step < MAX_STEP && consecutive_passes < 2) {
		std::vector<Point> moves = b->get_next_legal_moves();
		moves.push_back(Point(-1, -1));

		std::uniform_int_distribution<int> dist(0, moves.size() - 1);
		Point move = moves[dist(rng)];
		b->update_board(move);
		if (move.i == -1) {
			consecutive_passes++;
		} else {
			consecutive_passes = 0;
		}

		curr_step++;
	}

	int score = b->quick_score();
	if ((score > 0 && player == BLACK) || (score < 0 && player == WHITE)) {
		*wins += 1.0;
	}
	*sims += 1.0;
}

void Mcts::run_iteration(TreeNode* root, Board* curr_board, int& num_games) {
    TreeNode* node = root;

	std::vector<TreeNode*>* children = node->get_children();

	// while node is expandable and children not empty
    while(children != nullptr && !children->empty()) {
        node = selection(node);

		// apply virtual loss to discourage other threads 
		#pragma omp atomic
		node->sims += VIRTUAL_LOSS;

        curr_board->update_board(node->get_move());
		children = node->get_children();
    }

	// expand if expandable
	if (children == nullptr) {
		expand(node, curr_board);
		children = node->get_children();
	}

	// if children is not empty
	if (children != nullptr && !children->empty()) {
		thread_local std::mt19937 rng(std::random_device{}());
		std::uniform_int_distribution<int> dist(0, children.size() - 1);
		node = (*children)[dist(rng)]; 

		apply_virtual_loss(node);

		curr_board->update_board(node->get_move());
	}

	// runs one simulation
	double wins = 0.0;
	double sims = 0.0;
    run_simulation(curr_board, &wins, &sims);

    backprop(node, wins, sims);
}

bool Mcts::checkAbort() {
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