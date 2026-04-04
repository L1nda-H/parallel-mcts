
#ifndef GO_H
#define GO_H

#include "point.h"
#include "string.h"
#include "common.h"
#include <deque>
#include <vector>
#include <string.h>

enum COLOR {WHITE = 1, BLACK = 2, EMPTY = 0, OUT = 3};

class Board {
private:
	int dir[4][2] = {{1, 0}, {0, 1}, { -1, 0}, {0, -1}};
	int *board;  	// 1-d array to represent 2d board
	bool canEat(int i, int j, COLOR color);
	bool isSuicide(int i, int j, COLOR color);
	int remain;
	COLOR player;	// current player

public:
	Board() {
		int total = (BSIZE + 2) * (BSIZE + 2);
		board = new int[total];

		memset(board, 0, sizeof(int) * total);

		//set the border
		for (int i = 0; i < BSIZE + 2; i++) {
			board[i * (BSIZE + 2)] = 3;
			board[i * (BSIZE + 2) + BSIZE + 1] = 3;
			board[i] = 3;
			board[(BSIZE + 2) * (BSIZE + 1) + i] = 3;
		}

		player = BLACK; // black play first

		remain = BSIZE * BSIZE;
	}

	//copy constructor
	Board(const Board& b) {
		int total = (BSIZE + 2) * (BSIZE + 2);
		board = new int[total];

		for (int i = 0; i < BSIZE + 2; i++) {
			for (int j = 0; j < BSIZE + 2; j++) {
				board[i * (BSIZE + 2) + j] = b.getBoard(i, j);
			}
		}

		player = b.ToPlay();
		remain = b.getRemain();
	}

	void clear() {
		int total = (BSIZE + 2) * (BSIZE + 2);
		
		memset(board, 0, sizeof(int) * total);
		remain = BSIZE * BSIZE;

		//set the border
		for (int i = 0; i < BSIZE + 2; i++) {
			board[i * (BSIZE + 2)] = 3;
			board[i * (BSIZE + 2) + BSIZE + 1] = 3;
			board[i] = 3;
			board[(BSIZE + 2) * (BSIZE + 1) + i] = 3;
		}

		player = BLACK; // black play first
	}

	~Board() {
		delete []board;
	}

	void print_board();

	std::vector<Point> get_next_legal_moves();

	int update_board(Point pos);

	int score();
	
	COLOR ToPlay() const {
		return player;
	}

	int getBoard(int i, int j) const {
		return board[i * (BSIZE + 2) + j];
	}

	void setBoard(int i, int j, COLOR c) {
		board[i * (BSIZE + 2) + j] = c;
	}

	int getRemain() const {
		return remain;
	}
};

#endif