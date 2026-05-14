#ifndef MCTS_H
#define MCTS_H

#include <vector>
#include <stack>
#include <cmath>
#include <atomic>

#include "point.h"
#include "omp.h"

class Board;

class TreeNode {
private:
	Point move;
	std::atomic<std::vector<TreeNode*>*> children_ptr;

public:
	double wins; // Number of wins reached from this node
	double sims; // Number of simulations done on this node
	TreeNode* parent;

	TreeNode(Point curr_move)
			:  wins(0.0), sims(0.0), parent(NULL) {
				move = curr_move;
				children_ptr.store(nullptr, std::memory_order_relaxed);
	}

	TreeNode()
			:  wins(0.0), sims(0.0), parent(NULL) {
				move = Point(-1,-1);
			children_ptr.store(nullptr, std::memory_order_relaxed);
	}

	~TreeNode() {
		std::vector<TreeNode*>* children = children_ptr.load(std::memory_order_relaxed);

		if (children != nullptr) {
			for (TreeNode* child : *children) {
				delete child;
			}
		}
		parent = NULL;

	}

	// Compare and swap
	bool try_set_children(std::vector<TreeNode*>* new_children) {
		std::vector<TreeNode*>* expected = nullptr;
		return children_ptr.compare_exchange_strong(expected, new_children, std::memory_order_acq_rel);
	}

	std::vector<TreeNode*>* get_children() {
		return children_ptr.load(std::memory_order_acquire);
	}

	Point get_move(){
		return move;
	}
};


class MctsEngine {
public:
    virtual Point run(Board* curr_board, int rank, int& num_games) = 0;
    
    virtual ~MctsEngine() {} 
};

class Mcts : public MctsEngine{
private:
	TreeNode* root;
	struct timespec start, end;
	double maxTime;
	bool abort; 

public:
	Mcts(double time) {
		root = new TreeNode();
		maxTime = time;
		abort = false;
	}

	Mcts(double time, Point move) {
		root = new TreeNode(move);
		maxTime = time;
		abort = false;
	}

	~Mcts() {
		delete root;
	}

	Point run(Board* curr_board, int rank, int& num_games) override;
	
	void run_iteration(TreeNode* node, Board* curr_board, int& num_games);
	TreeNode *selection(TreeNode* node);
	void expand(TreeNode* node, Board* board);
	void backprop(TreeNode* node, int win_increase, int sim_increase);

	bool checkAbort();
};

#endif