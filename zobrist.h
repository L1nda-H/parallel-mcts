#ifndef MCTS_ZOBRIST
#define MCTS_ZOBRIST

#include <random>
#include <vector>
#include <atomic>
#include <iostream>

#include "common.h"
#include "mcts.h"
#include "mpi.h"
#include "omp.h"

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
    omp_lock_t lock;
    Point move;
    bool expanded;
    double wins;
    double sims;

    TNode() : hash(0), timestamp(1), wins(0.0), sims(0.0) {
        omp_init_lock(&lock);
    }

    ~TNode() {
        omp_destroy_lock(&lock);
    }
};

struct TTableUpdate {
    uint64_t hash;
    double wins;
    double sims;
    uint64_t timestamp;
};

class TTable {
private:
    TNode* table;
    uint64_t table_size;
    int mpi_rank;
    int mpi_size;
    MPI_Comm table_comm;
    bool mpi_ready;
    std::vector<std::vector<TTableUpdate> > outgoing;
    uint64_t pending_updates;
    omp_lock_t outgoing_lock;
    bool io_running;
    std::atomic<bool> stop_requested;

    void updateLocalNode(uint64_t board_hash, double wins_to_add, double sims_to_add, uint64_t current_time);
    void updateRemoteNode(int dest, const TTableUpdate& update);
    void ioLoop();

public:
    TTable();

    ~TTable();

    TNode* getNode(uint64_t board_hash) {
        uint64_t index = board_hash % table_size;
        return &table[index];
    }

    int getOwner(uint64_t board_hash) const;
    bool owns(uint64_t board_hash) const;
    void startIoThread();
    void stopIoThread();
    void runIoThread();
    bool getStats(uint64_t board_hash, double* wins, double* sims);
    void updateLocalVirtualLoss(uint64_t board_hash, uint64_t current_time);
    void updateNode(uint64_t board_hash, double wins_to_add, double sims_to_add, uint64_t current_time);
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
