#include <stdio.h>
#include <time.h>
#include <random>
#include <algorithm>
#include <iostream>

#include "mcts.h"
#include "Go.h"
#include "point.h"

#define C 1.4
#define EPSILON 10e-64
#define MAX_STEP 100 // redefine to avoid repeat game
#define BILLION 1000000000L
#define MILLION 1000000.0

Point Mcts::run(Board* curr_board, int rank, int& num_games) {
	clock_gettime(CLOCK_REALTIME, &start);
	Board* curr_board_copy = new Board();
	while(!checkAbort()) {
		curr_board_copy->copy_board(curr_board);
		run_iteration(root, curr_board_copy, num_games);
	}

	double maxv = -1.0;
	TreeNode* best = NULL;
	std::vector<TreeNode*> children = root->get_children();
	for (std::vector<TreeNode*>::iterator it = children.begin(); it != children.end(); it++) {
		TreeNode* c = *it;
		double v = c->sims;
		if (v > maxv) {
			maxv = v;
			best = c;
		}
	}

	delete curr_board_copy;

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
		node->sims += sim_increase;
		if (lv)node->wins += win_increase;
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

void Mcts::run_iteration(TreeNode* root, Board* curr_board, int& num_games) {
    TreeNode* node = root;

    while(!node->is_expandable() && !node->get_children().empty()) {
        node = selection(node);
        curr_board->update_board(node->get_move());
    }

    if (node->is_expandable()) {
        expand(node, curr_board);
        node->set_expandable(false);

		thread_local std::mt19937 rng(std::random_device{}());

		std::vector<TreeNode*> children = node->get_children();

        if (!children.empty()) {
			std::uniform_int_distribution<int> dist(0, children.size() - 1);
            node = children[dist(rng)]; 
			curr_board->update_board(node->get_move());
        }
    }

	// runs one simulation
	double wins = 0.0;
	double sims = 0.0;
    run_simulation(curr_board, &wins, &sims);
	num_games += 1;

    backprop(node, wins, sims);
}

bool Mcts::checkAbort() {
	if (!abort) {
		u_int64_t diff;
		clock_gettime(CLOCK_REALTIME, &end);
		diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
		abort = diff / MILLION > maxTime;
	}
	return abort;
}