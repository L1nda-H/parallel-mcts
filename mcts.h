#ifndef MCTS_H
#define MCTS_H

#include <vector>
#include <stack>
#include <cmath>

#include "point.h"
#include "Go.h"

class TreeNode {
private:
	Point move;
	std::vector<TreeNode*> children;
	bool expandable;     // unexpanded

public:
	double wins; // Number of wins reached from this node
	double sims; // Number of simulations done on this node
	TreeNode* parent;
	TreeNode(Point curr_move)
			:  expandable(true), wins(0.0), sims(0.0), parent(NULL) {
				move = curr_move;
			
	}

	TreeNode()
			:  expandable(true), wins(0.0), sims(0.0), parent(NULL) {
				move = Point(-1,-1);
	}

	~TreeNode() {
		for (std::vector<TreeNode*>::iterator it = children.begin(); it != children.end(); it++) {
			delete *it;
		}
		parent = NULL;
	}

	bool is_expandable() {
		return expandable;
	}
	void set_expandable(bool b) {
		expandable = b;
	}

	void add_children(TreeNode* child){
		children.push_back(child);
		child->parent = this;
	}
	std::vector<TreeNode*> get_children() {
		return children;
	}

	Point get_move(){
		return move;
	}
};


class Mcts {
private:
	TreeNode* root;
	double maxTime;
	bool abort; 
	int bd_size;

public:
	Mcts(int size, double time) {
		bd_size = size;
		root = new TreeNode();
		maxTime = time;
	}

	Mcts(int size, double time, Point move) {
		bd_size = size;
		root = new TreeNode(move);
		maxTime = time;
	}

	~Mcts() {
		delete root;
	}

	Point run();
	
	void run_iteration(TreeNode* node);
	TreeNode *selection(TreeNode* node);
	void expand(TreeNode* node, Board* board);
	void backprop(TreeNode* node, int win_increase, int sim_increase);

	Board* get_board(TreeNode* node, Point* move);
	bool checkAbort();
	void update(TreeNode* node, double* win, double* sim,  int incre, int thread_num);
};

#endif