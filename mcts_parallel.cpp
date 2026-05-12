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

	double local_sims[num_possible_moves] = {0.0};

	std::vector<TreeNode*>* children = root->get_children();

	if (children != nullptr) {
		for (TreeNode* c : *children) {
			int id = Point::point_to_id(c->get_move(), bsize);
			if(id > -1) {
				local_sims[id] = c->sims;
			}
		}
	}

	double global_sims[num_possible_moves] = {0.0};

	MPI_Reduce(local_sims, global_sims, num_possible_moves, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

	int best_id = -1;
	
	if(rank == 0){
		double maxv = 0;
		for(int i = 0; i < num_possible_moves; i++){
			double v = global_sims[i];
			if(v > maxv){
				maxv = v;
				best_id = i;
			}
		}
	}

	MPI_Bcast(&best_id, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if(rank == 0){
		num_games += std::accumulate(global_sims, global_sims + num_possible_moves, 0);
	}

	return Point::id_to_point(best_id, bsize);
}

TreeNode* Mcts::selection(TreeNode* node) {
	double maxv = -1.0;
	TreeNode* maxn = NULL;
	double n = node->sims;

	if (n < 1.0) {
		n = 1.0;
	}

	std::vector<TreeNode*>* children = node->get_children();

	if (children == nullptr) return node;

	for (TreeNode* c : *children) {
		if (c->sims == 0.0) {
			return c;
		}
		double v = c->wins / (c->sims + EPSILON) + C * sqrt(log(n + EPSILON) / (c->sims + EPSILON));
		if (v > maxv) {
			maxv = v;
			maxn = c;
		}
	}
	return maxn;
}

void Mcts::expand(TreeNode* node, Board* board) {
	std::vector<Point> moves_vec = board->get_next_legal_moves();
	
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

void Mcts::run_iteration(TreeNode* root, Board* curr_board, int& num_games) {
    TreeNode* node = root;

	#pragma omp atomic
	node->sims += VIRTUAL_LOSS;

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
		std::uniform_int_distribution<int> dist(0, children->size() - 1);
		node = (*children)[dist(rng)];

		#pragma omp atomic
		node->sims += VIRTUAL_LOSS;

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