#ifndef MCTS_ZOBRIST
#define MCTS_ZOBRIST

#include <random>
#include <vector>
#include <atomic>
#include <iostream>

#include "common.h"
#include "mcts.h"

class Board;

struct ZobristHash {
    std::vector<uint64_t> wTable;
    std::vector<uint64_t> bTable;

    ZobristHash() {};

    void buildTable (int bsize) {
        std::random_device rd;
        std::mt19937_64 gen(rd());

        std::uniform_int_distribution<uint64_t> dis;

        wTable.resize(bsize * bsize);
        bTable.resize(bsize * bsize);
        for (int i = 0; i < bsize; i++) {
            for (int j = 0; j < bsize; j++) {
                wTable[i * bsize + j] = dis(gen);
                bTable[i * bsize + j] = dis(gen);
            }
        }
    }

    uint64_t updateHash(uint64_t current_hash, int pos, COLOR color) {
        if (color == OUT) {
            return current_hash;
        }
        if (color == WHITE) {
            return current_hash ^= wTable[pos];
        }
        return current_hash ^= bTable[pos];
    }
};

struct TNode {
    uint64_t hash;
    std::atomic<uint64_t> timestamp;
    Point move;
    bool expanded;
    double wins;
    double sims;

    TNode() : hash(0), timestamp(1), wins(0.0), sims(0.0) {};
};

class TTable {
private:
    TNode* table;
    uint64_t table_size;
public:
    TTable() {
        table_size = TRANSPOSITION_TABLE_SIZE;
        table = new TNode[TRANSPOSITION_TABLE_SIZE];
    }

    ~TTable() {
        delete[] table;
    }

    TNode* getNode(uint64_t board_hash) {
        uint64_t index = board_hash % table_size;
        return &table[index];
    }

    void updateNode(uint64_t board_hash, double wins_to_add, double sims_to_add, uint64_t current_time) {
        uint64_t index = board_hash % table_size;
        TNode& node = table[index];
        uint64_t expected_time = node.timestamp.load();
        
        while (expected_time == 0 || !node.timestamp.compare_exchange_weak(expected_time, 0)) {
            expected_time = node.timestamp.load();
        }

        if (node.hash != board_hash) {
            // std::cerr << "clearing board for hash " << board_hash << "\n";
            // fflush(stderr);
            node.hash = board_hash;
            node.wins = 0;
            node.sims = 0;
        }

        // std::cerr << "adding " << sims_to_add << " for hash " << board_hash << "\n";
        // fflush(stderr);
        node.wins += wins_to_add;
        node.sims += sims_to_add;

        node.timestamp.store(current_time);
    }
};

class Mcts_zobrist : public MctsEngine{
private:
    TTable* table;
    struct timespec start, end;
    int move;
    double maxTime;
    bool abort;
    COLOR player;

public:
    Mcts_zobrist(double time, TTable* global_table, int m) {
        table = global_table;
        maxTime = time;
        move = m;
        abort = false;
    }

    ~Mcts_zobrist() {}

    Point run(Board* curr_board, int rank, int& num_games) override;
	
	void run_iteration(Board* curr_board, Board* scratch_board, int rank, int& num_games);

    void simulate(Board* b, double* wins, double* sims);
    
	void backprop(const std::vector<uint64_t>& search_path, double wins, double sims);

	bool checkAbort();
};

#endif