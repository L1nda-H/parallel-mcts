#include <stdio.h>

#include "mcts.h"
#include "Go.h"
#include "point.h"

#define C 1.4
#define EPSILON 10e-6

Point Mcts::run(Board* curr_board) {
	Board* curr_board_copy = new Board();
	while(!checkAbort()) {
		curr_board_copy->copy_board(curr_board);
		run_iteration(root, curr_board_copy);
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
	while (moves_vec.size() > 0) {
		Point nxt_move = moves_vec.back();
		node->add_children(new TreeNode(nxt_move));
		moves_vec.pop_back();
	}
}

void Mcts::backprop(TreeNode* node, int win_increase, int sim_increase) {
	bool lv = false;
	while (node->parent != NULL) {
		node = node->parent;
		node->sims += sim_increase;
		if (lv)node->wins += win_increase;
		lv = !lv;
	}
}

void Mcts::run_iteration(TreeNode* root, Board* curr_board) {
    TreeNode* node = root;
    curr_board->update_board(root->get_move());

    while(!node->is_expandable()) {
        node = selection(node);
        curr_board->update_board(node->get_move());
    }

    if (node->is_expandable()) {
        expand(node, curr_board);
        delete curr_board;
        node->set_expandable(false);

        if (!node->get_children().empty()) {
            node = node->get_children()[0]; // TODO: this should select random number
        }
    }

    double result = run_simulation();

    backprop(node, node->wins, node->sims);
}

double run_simulation() {

}