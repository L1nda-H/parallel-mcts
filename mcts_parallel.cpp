#include <stdio.h>
#include <time.h>
#include <random>
#include <algorithm>

#include "mcts.h"
#include "Go.h"
#include "point.h"
#include "omp.h"

#define C 1.4
#define EPSILON 10e-64
#define MAX_STEP 100 // redefine to avoid repeat game
#define BILLION 1000000000L
#define MILLION 1000000.0
#define VIRTUAL_LOSS 1.0
#define NUM_THREADS 32


Point Mcts::run(Board* curr_board) {
	clock_gettime(CLOCK_REALTIME, &start);
	#pragma omp parallel
	{
		Board* curr_board_copy = new Board();
		while(!checkAbort()) {
			curr_board_copy->copy_board(curr_board);
			run_iteration(root, curr_board_copy);
		}
		delete curr_board_copy;
	}

	double maxv = -1.0;
	TreeNode* best = NULL;
	std::vector<TreeNode*> children = root->get_children();
	for (std::vector<TreeNode*>::iterator it = children.begin(); it != children.end(); it++) {
		TreeNode* c = *it;
		double v = c->wins / (c->sims + EPSILON);
		if (v > maxv) {
			maxv = v;
			best = c;
		}
	}

	if (best == NULL) {
		return Point(-1,-1);
	}
	return best->get_move();
}

TreeNode* Mcts::selection(TreeNode* node) {
	double maxv = -1.0;
	TreeNode* maxn = NULL;
	double n = node->sims;

	std::vector<TreeNode*> children = node->get_children();
	for (std::vector<TreeNode*>::iterator it = children.begin(); it != children.end(); it++) {
		TreeNode* c = *it;
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

	thread_local std::mt19937 rng(std::random_device{}());
    
    std::shuffle(moves_vec.begin(), moves_vec.end(), rng);
	
	while (moves_vec.size() > 0) {
		Point nxt_move = moves_vec.back();
		node->add_children(new TreeNode(nxt_move));
		moves_vec.pop_back();
	}
}

void Mcts::backprop(TreeNode* node, int win_increase, int sim_increase) {
	bool lv = false;
	while (node != NULL) {
		#pragma omp atomic
		node->sims += (sim_increase - VIRTUAL_LOSS);

		if (lv) {
			#pragma omp atomic
			node->wins += win_increase;
		}

		node = node->parent;
		lv = !lv;
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

void Mcts::run_iteration(TreeNode* root, Board* curr_board) {
    TreeNode* node = root;

	#pragma omp atomic
	node->sims += VIRTUAL_LOSS;

    while(!node->is_expandable() && !node->get_children().empty()) {
        node = selection(node);

		// apply virtual loss to discourage other threads 
		#pragma omp atomic
		node->sims += VIRTUAL_LOSS;


        curr_board->update_board(node->get_move());
    }

    if (node->is_expandable()) {
        omp_set_lock(&node->lock);
		// double check if another thread already expanded the node while this thread was waiting for the lock
		if(node->is_expandable()){
			expand(node, curr_board);
			node->set_expandable(false);
		}
		omp_unset_lock(&node->lock);


		thread_local std::mt19937 rng(std::random_device{}());

		std::vector<TreeNode*> children = node->get_children();

        if (!children.empty()) {
			std::uniform_int_distribution<int> dist(0, children.size() - 1);
            node = children[dist(rng)]; 

			// apply virtual loss to the newly selected child
			#pragma omp atomic
			node->sims += VIRTUAL_LOSS;

			curr_board->update_board(node->get_move());
        }
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