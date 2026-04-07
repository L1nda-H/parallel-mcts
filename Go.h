
#ifndef GO_H
#define GO_H

#include "point.h"
#include "string.h"
#include "common.h"
#include <deque>
#include <vector>
#include <string.h>
#include <cstring>

enum COLOR {WHITE = 1, BLACK = 2, EMPTY = 0, OUT = 3};

class Board {
private:
	int dir[4][2] = {{1, 0}, {0, 1}, { -1, 0}, {0, -1}};
	std::vector<int> board; 
	bool canEat(int i, int j, COLOR color);
	bool isSuicide(int i, int j, COLOR color);
	COLOR player;	// current player

public:
	Board() {
		board.resize(BSIZEIDX * BSIZEIDX);
		//set the border
		for (int i = 0; i < BSIZE + 2; i++) {
			board[i * (BSIZE + 2)] = 3;
			board[i * (BSIZE + 2) + BSIZE + 1] = 3;
			board[i] = 3;
			board[(BSIZE + 2) * (BSIZE + 1) + i] = 3;
		}

		player = BLACK; // black play first
	}

	//copy constructor
	Board(const Board& b) {
		board.resize(BSIZEIDX * BSIZEIDX);
		for (int i = 0; i < BSIZE + 2; i++) {
			for (int j = 0; j < BSIZE + 2; j++) {
				board[i * (BSIZE + 2) + j] = b.getBoard(i, j);
			}
		}

		player = b.ToPlay();
	}

	void clear() {
		std::fill(board.begin(), board.end(), 0);

		//set the border
		for (int i = 0; i < BSIZE + 2; i++) {
			board[i * (BSIZE + 2)] = 3;
			board[i * (BSIZE + 2) + BSIZE + 1] = 3;
			board[i] = 3;
			board[(BSIZE + 2) * (BSIZE + 1) + i] = 3;
		}

		player = BLACK; // black play first
	}

	void print_board();

	std::vector<Point> get_next_legal_moves();

	int update_board(Point pos);

	int quick_score();
	
	COLOR ToPlay() const {
		return player;
	}

	int getBoard(int i, int j) const {
		return board[i * (BSIZE + 2) + j];
	}

	void setBoard(int i, int j, COLOR c) {
		board[i * (BSIZE + 2) + j] = c;
	}

	void copy_board(const Board* other) {
		this->player = other->player;
		
		std::memcpy(this->board.data(), other->board.data(), this->board.size() * sizeof(int));
	}
};

#endif