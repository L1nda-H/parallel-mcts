#include "zobrist.h"
#include <time.h>

namespace {
const int TT_UPDATE_TAG = 9101;
const int TT_DONE_TAG = 9102;

struct PendingSend {
    std::vector<TTableUpdate> updates;
    MPI_Request request;
};

uint64_t acquireNode(TNode& node) {
    uint64_t expected_time = node.timestamp.load();
    while (expected_time == 0 || !node.timestamp.compare_exchange_weak(expected_time, 0)) {
        expected_time = node.timestamp.load();
    }
    return expected_time;
}
}

TTable::TTable() {
    table_size = TRANSPOSITION_TABLE_SIZE;
    table = new TNode[TRANSPOSITION_TABLE_SIZE];
    mpi_rank = 0;
    mpi_size = 1;
    table_comm = MPI_COMM_WORLD;
    mpi_ready = false;
    pending_updates = 0;
    io_running = false;
    stop_requested = false;
    omp_init_lock(&outgoing_lock);

    int initialized = 0;
    MPI_Initialized(&initialized);
    if (initialized) {
        mpi_ready = true;
        MPI_Comm_dup(MPI_COMM_WORLD, &table_comm);
        MPI_Comm_rank(table_comm, &mpi_rank);
        MPI_Comm_size(table_comm, &mpi_size);
    }
    outgoing.resize(mpi_size);
}

TTable::~TTable() {
    stopIoThread();
    omp_destroy_lock(&outgoing_lock);
    int initialized = 0;
    int finalized = 0;
    MPI_Initialized(&initialized);
    MPI_Finalized(&finalized);
    if (initialized && !finalized && table_comm != MPI_COMM_WORLD && table_comm != MPI_COMM_NULL) {
        MPI_Comm_free(&table_comm);
    }
    delete[] table;
}

int TTable::getOwner(uint64_t board_hash) const {
    if (!mpi_ready || mpi_size <= 1) {
        return 0;
    }
    return static_cast<int>(board_hash % static_cast<uint64_t>(mpi_size));
}

bool TTable::owns(uint64_t board_hash) const {
    return getOwner(board_hash) == mpi_rank;
}

void TTable::startIoThread() {
    if (!mpi_ready || mpi_size <= 1) {
        return;
    }

    omp_set_lock(&outgoing_lock);
    if (io_running) {
        omp_unset_lock(&outgoing_lock);
        return;
    }
    stop_requested = false;
    pending_updates = 0;
    for (std::vector<TTableUpdate>& batch : outgoing) {
        batch.clear();
    }
    io_running = true;
    omp_unset_lock(&outgoing_lock);
}

void TTable::stopIoThread() {
    omp_set_lock(&outgoing_lock);
    if (!io_running) {
        omp_unset_lock(&outgoing_lock);
        return;
    }
    stop_requested = true;
    omp_unset_lock(&outgoing_lock);
}

void TTable::runIoThread() {
    if (!mpi_ready || mpi_size <= 1) {
        return;
    }
    ioLoop();
}

bool TTable::getStats(uint64_t board_hash, double* wins, double* sims) {
    if (!owns(board_hash)) {
        *wins = 0.0;
        *sims = 0.0;
        return false;
    }

    TNode* node = getNode(board_hash);
    uint64_t expected_time = acquireNode(*node);

    bool found = node->hash == board_hash;
    if (found) {
        *wins = node->wins;
        *sims = node->sims;
    } else {
        *wins = 0.0;
        *sims = 0.0;
    }
    node->timestamp.store(expected_time);
    return found;
}

void TTable::updateLocalNode(uint64_t board_hash, double wins_to_add, double sims_to_add, uint64_t current_time) {
    uint64_t index = board_hash % table_size;
    TNode& node = table[index];
    acquireNode(node);

    if (node.hash != board_hash) {
        node.hash = board_hash;
        node.wins = 0;
        node.sims = 0;
    }

    node.wins += wins_to_add;
    node.sims += sims_to_add;

    node.timestamp.store(current_time);
}

void TTable::updateRemoteNode(int dest, const TTableUpdate& update) {
    if (dest < 0 || dest >= mpi_size || dest == mpi_rank) {
        updateLocalNode(update.hash, update.wins, update.sims, update.timestamp);
        return;
    }

    omp_set_lock(&outgoing_lock);
    outgoing[dest].push_back(update);
    pending_updates++;
    omp_unset_lock(&outgoing_lock);
}

void TTable::updateLocalVirtualLoss(uint64_t board_hash, uint64_t current_time) {
    if (!owns(board_hash)) {
        return;
    }

    updateLocalNode(board_hash, 0.0, VIRTUAL_LOSS, current_time);
}

void TTable::updateNode(uint64_t board_hash, double wins_to_add, double sims_to_add, uint64_t current_time) {
    int dest = getOwner(board_hash);
    if (dest == mpi_rank || !mpi_ready || mpi_size <= 1) {
        updateLocalNode(board_hash, wins_to_add, sims_to_add, current_time);
        return;
    }

    TTableUpdate update;
    update.hash = board_hash;
    update.wins = wins_to_add;
    update.sims = sims_to_add;
    update.timestamp = current_time;
    updateRemoteNode(dest, update);
}

void TTable::ioLoop() {
    std::vector<bool> done_from(mpi_size, false);
    std::vector<PendingSend> pending_sends;
    bool sent_done = false;

    while (true) {
        for (std::vector<PendingSend>::iterator it = pending_sends.begin(); it != pending_sends.end();) {
            int complete = 0;
            MPI_Test(&it->request, &complete, MPI_STATUS_IGNORE);
            if (complete) {
                it = pending_sends.erase(it);
            } else {
                ++it;
            }
        }

        bool handled_message = false;
        int flag = 0;
        MPI_Status status;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, table_comm, &flag, &status);
        // receive updates loop
        while (flag) {
            handled_message = true;
            if (status.MPI_TAG == TT_UPDATE_TAG) {
                int byte_count = 0;
                MPI_Get_count(&status, MPI_BYTE, &byte_count);
                std::vector<TTableUpdate> updates(byte_count / static_cast<int>(sizeof(TTableUpdate)));
                MPI_Recv(updates.data(), byte_count, MPI_BYTE, status.MPI_SOURCE, TT_UPDATE_TAG, table_comm, MPI_STATUS_IGNORE);
                for (const TTableUpdate& update : updates) {
                    updateLocalNode(update.hash, update.wins, update.sims, update.timestamp);
                }
            } else if (status.MPI_TAG == TT_DONE_TAG) {
                MPI_Recv(NULL, 0, MPI_BYTE, status.MPI_SOURCE, TT_DONE_TAG, table_comm, MPI_STATUS_IGNORE);
                done_from[status.MPI_SOURCE] = true;
            } else {
                MPI_Recv(NULL, 0, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, table_comm, MPI_STATUS_IGNORE);
            }
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, table_comm, &flag, &status);
        }

        std::vector<std::vector<TTableUpdate>> batches(mpi_size);
        bool should_send = false;
        bool should_stop = false;
        omp_set_lock(&outgoing_lock);
        should_stop = stop_requested;
        should_send = pending_updates >= TT_REMOTE_BATCH_THRESHOLD || (should_stop && pending_updates > 0);
        if (should_send) {
            for (int dest = 0; dest < mpi_size; dest++) {
                if (dest == mpi_rank || outgoing[dest].empty()) {
                    continue;
                }
                batches[dest].swap(outgoing[dest]);
                pending_updates -= batches[dest].size();
            }
        }
        omp_unset_lock(&outgoing_lock);

        for (int dest = 0; dest < mpi_size; dest++) {
            if (dest == mpi_rank || batches[dest].empty()) {
                continue;
            }
            pending_sends.push_back(PendingSend());
            PendingSend& send = pending_sends.back();
            send.updates.swap(batches[dest]);
            int bytes = static_cast<int>(send.updates.size() * sizeof(TTableUpdate));
            MPI_Isend(send.updates.data(), bytes, MPI_BYTE, dest, TT_UPDATE_TAG, table_comm, &send.request);
        }

        omp_set_lock(&outgoing_lock);
        should_stop = stop_requested && pending_updates == 0;
        omp_unset_lock(&outgoing_lock);

        if (should_stop && !sent_done) {
            for (int dest = 0; dest < mpi_size; dest++) {
                if (dest == mpi_rank) {
                    continue;
                }
                pending_sends.push_back(PendingSend());
                PendingSend& send = pending_sends.back();
                MPI_Isend(NULL, 0, MPI_BYTE, dest, TT_DONE_TAG, table_comm, &send.request);
            }
            sent_done = true;
        }

        bool all_done = true;
        for (int src = 0; src < mpi_size; src++) {
            if (src != mpi_rank && !done_from[src]) {
                all_done = false;
                break;
            }
        }
        if (sent_done && all_done && pending_sends.empty()) {
            break;
        }

        if (!handled_message && !should_send) {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 1000000;
            nanosleep(&ts, NULL);
        }
    }

    omp_set_lock(&outgoing_lock);
    io_running = false;
    stop_requested = false;
    omp_unset_lock(&outgoing_lock);
}
