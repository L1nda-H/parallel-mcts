#ifndef GO_H
#define GO_H

#include "point.h"
#include "string.h"
#include "zobrist.h"
#include "common.h"

#include <deque>
#include <vector>
#include <string.h>
#include <cstring>
#include <stdexcept>

class Board {
private:
	int dir[4][2] = {{1, 0}, {0, 1}, { -1, 0}, {0, -1}};
	std::vector<COLOR> board; 
	bool canEat(int i, int j, COLOR color);
	bool isSuicide(int i, int j, COLOR color);
	bool zobrist;
	ZobristHash hash_table;
	uint64_t state_hash;
	uint64_t prev_state_hash;
	int countLiberties(int i, int j, COLOR color);
	COLOR player;	// current player
	int bsize;
	int bsize_idx;
	Point ko = Point(-1, -1);

public:
	Board(int board_size) {
		bsize = board_size;
		bsize_idx = board_size + 2;
		board.resize(bsize_idx * bsize_idx);
		//set the border
		for (int i = 0; i < bsize_idx; i++) {
			board[i * (bsize_idx)] = OUT;
			board[i * (bsize_idx) + bsize + 1] = OUT;
			board[i] = OUT;
			board[(bsize_idx) * (bsize + 1) + i] = OUT;
		}
		zobrist = false;
		state_hash = 0;
		prev_state_hash = 0;
		player = COLOR::BLACK; // black play first
	}

	Board(int board_size, ZobristHash ht) {
		bsize = board_size;
		bsize_idx = board_size + 2;
		board.resize(bsize_idx * bsize_idx);
		//set the border
		for (int i = 0; i < bsize_idx; i++) {
			board[i * (bsize_idx)] = OUT;
			board[i * (bsize_idx) + bsize + 1] = OUT;
			board[i] = OUT;
			board[(bsize_idx) * (bsize + 1) + i] = OUT;
		}
		zobrist = true;
		hash_table = ht;
		state_hash = 0;
		prev_state_hash = 0;

		player = COLOR::BLACK; // black play first
	}

	void clear() {
		std::fill(board.begin(), board.end(), EMPTY);

		//set the border
		for (int i = 0; i < bsize_idx; i++) {
			board[i * (bsize_idx)] = OUT;
			board[i * (bsize_idx) + bsize + 1] = OUT;
			board[i] = OUT;
			board[(bsize_idx) * (bsize + 1) + i] = OUT;
		}

		player = COLOR::BLACK; // black play first
		state_hash = 0;
		prev_state_hash = 0;
	}

	void print_board();

	std::vector<Point> get_next_legal_moves();

	int update_board(Point pos);

	int quick_score();
	
	COLOR ToPlay() const {
		return player;
	}

	COLOR getBoard(int i, int j) const {
		return board[i * (bsize_idx) + j];
	}

	void setBoard(int i, int j, COLOR c) {
		board[i * (bsize_idx) + j] = c;
	}

	void copy_board(const Board* other) {
		this->player = other->player;
		this->state_hash = other->state_hash;
		this->prev_state_hash = other->prev_state_hash;
		this->zobrist = other->zobrist;
		
		std::memcpy(this->board.data(), other->board.data(), this->board.size() * sizeof(COLOR));
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
