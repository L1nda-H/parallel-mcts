#ifndef MCTS_ZOBRIST
#define MCTS_ZOBRIST

#include <random>
#include <vector>
#include <atomic>

#include "common.h"
#include "mcts.h"

class Board;

struct ZobristHash {
    std::vector<uint64_t> table;

    ZobristHash() {};

    void buildTable (int bsize_idx) {
        std::random_device rd;
        std::mt19937_64 gen(rd());

        std::uniform_int_distribution<uint64_t> dis;

        table.resize(2 * bsize_idx * bsize_idx);
        for (int i = 1; i < table.size() - 1; i++) {
            table[i] = dis(gen);
        }
    }

    uint64_t generateInitHash(std::vector<int>& b) {
        uint64_t hash = 0;
        for(int i = 1; i < b.size() - 1; i++) {
            if (b[i] != EMPTY && b[i] != OUT) {
                hash = hash ^= table[b[i] - 1 + i * 2];
            }
        }
        return hash;
    }

    uint64_t updateHash(uint64_t current_hash, int pos, COLOR color) {
        if (color == EMPTY || color == OUT) {
            return current_hash;
        }
        return current_hash ^= table[pos * 2 + color - 1];
    }
};

struct TNode {
    uint64_t hash;
    std::atomic<uint64_t> timestamp;
    Point move;
    bool expanded;
    double wins;
    double sims;

    TNode() : hash(0), timestamp(0), wins(0.0), sims(0.0) {};
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
            // is lazy overwriting of old game state an issue? 
            node.hash = board_hash;
            node.wins = 0;
            node.sims = 0;
        }

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

public:
    Mcts_zobrist(double time, TTable* global_table, int m) {
        table = global_table;
        maxTime = time;
        move = m;
    }

    ~Mcts_zobrist() {}

    Point run(Board* curr_board, int rank, int& num_games) override;
	
	void run_iteration(Board* curr_board, int& num_games);
    
	void backprop(const std::vector<uint64_t>& search_path, double wins, double sims);

	bool checkAbort();
};

#endif