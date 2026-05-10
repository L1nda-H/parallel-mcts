#ifndef GO_H
#define GO_H

#include "point.h"
#include "string.h"
#include "zobrist.h"
#include <deque>
#include <vector>
#include <string.h>
#include <cstring>
#include <stdexcept>

// TODO pachi has a different concept of komi, so we might just remove this
#define KOMI 0

enum COLOR {WHITE = 1, BLACK = 2, EMPTY = 0, OUT = 3};

class Board {
private:
	int dir[4][2] = {{1, 0}, {0, 1}, { -1, 0}, {0, -1}};
	std::vector<int> board; 
	bool canEat(int i, int j, COLOR color);
	bool isSuicide(int i, int j, COLOR color);
	ZobristHash hash_table;
	uint64_t state_hash;
	int countLiberties(int i, int j, COLOR color);
	COLOR player;	// current player
	int bsize;
	int bsize_idx;

public:
	Board(int board_size) {
		bsize = board_size;
		bsize_idx = board_size + 2;
		board.resize(bsize_idx * bsize_idx);
		//set the border
		for (int i = 0; i < bsize_idx; i++) {
			board[i * (bsize_idx)] = 3;
			board[i * (bsize_idx) + bsize + 1] = 3;
			board[i] = 3;
			board[(bsize_idx) * (bsize + 1) + i] = 3;
		}

		player = BLACK; // black play first
	}

	Board(int board_size, ZobristHash ht) {
		bsize = board_size;
		bsize_idx = board_size + 2;
		board.resize(bsize_idx * bsize_idx);
		//set the border
		for (int i = 0; i < bsize_idx; i++) {
			board[i * (bsize_idx)] = 3;
			board[i * (bsize_idx) + bsize + 1] = 3;
			board[i] = 3;
			board[(bsize_idx) * (bsize + 1) + i] = 3;
		}
		hash_table = ht;
		state_hash = ht.generateInitHash(board);

		player = BLACK; // black play first
	}

	//copy constructor
	Board(const Board& b) {
		bsize = b.bsize;
		bsize_idx = b.bsize_idx;
		board.resize(bsize_idx * bsize_idx);
		for (int i = 0; i < bsize_idx; i++) {
			for (int j = 0; j < bsize_idx; j++) {
				board[i * (bsize_idx) + j] = b.getBoard(i, j);
			}
		}
		hash_table = b.hash_table;
		state_hash = b.state_hash;

		player = b.ToPlay();
	}

	void clear() {
		std::fill(board.begin(), board.end(), 0);

		//set the border
		for (int i = 0; i < bsize_idx; i++) {
			board[i * (bsize_idx)] = 3;
			board[i * (bsize_idx) + bsize + 1] = 3;
			board[i] = 3;
			board[(bsize_idx) * (bsize + 1) + i] = 3;
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
		return board[i * (bsize_idx) + j];
	}

	void setBoard(int i, int j, COLOR c) {
		board[i * (bsize_idx) + j] = c;
		state_hash = hash_table.updateHash(state_hash, i * (bsize_idx) + j, c);
	}

	void copy_board(const Board* other) {
		this->player = other->player;
		
		std::memcpy(this->board.data(), other->board.data(), this->board.size() * sizeof(int));
	}

	int get_bsize() {
		return bsize;
	}

	uint64_t get_hash() {
		return state_hash;
	}

	ZobristHash get_htable() {
		return hash_table;
	}
};

#endif