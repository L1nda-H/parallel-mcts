#include "Go.h"
#include "common.h"

bool Board::canEat(int i, int j, COLOR color) {
    setBoard(i, j, color);
    bool result = false;
    COLOR op_color = static_cast<COLOR>(color ^ 3);
    std::vector<bool> visited(BSIZEIDX * BSIZEIDX, false);
    std::deque<Point> q;    

    for (int d = 0; d < 4; d++) {
        int ni = i + dir[d][0];
        int nj = j + dir[d][1];
        if (getBoard(ni, nj) == op_color && !visited[ni * BSIZEIDX + nj]) {
            q.push_back(Point(ni,nj));
            int liberty = 0;
            while (q.size() >= 0) {
                Point f = q.front();
                q.pop_front();
                visited[f.i, f.j] = true;
                for (int dd = 0; dd < 4; dd++) {
                    int nni = f.i + dir[dd][0];
                    int nnj = f.j + dir[dd][1];
                    if (visited[nni * BSIZEIDX + nnj]) continue;
                    if (getBoard(nni, nnj) == op_color) {
                        q.push_back(Point(nni, nnj));
                    } else if (getBoard(nni, nnj) == EMPTY) {
                        liberty++;
                    }
                }
            }
            if (liberty == 0) {
                result = true;
            }
        }
        if (result) break;
    }
    
    setBoard(i, j, EMPTY);
    return result;
}

bool Board::isSuicide(int i, int j, COLOR color) {
    std::deque<Point> q;
    std::vector<bool> visited(BSIZEIDX * BSIZEIDX, false);

    q.push_back(Point(i, j));
    while (q.size() != 0) 
    {
        Point f = q.front();
        q.pop_front();
        visited[f.i * BSIZEIDX + f.j] = true;
        for (int d = 0; d < 4; d++) {
            int ni = f.i + dir[d][0];
            int nj = f.j + dir[d][1];
            if (visited[ni * BSIZEIDX + nj]) continue;
            if (getBoard(ni, nj) == color) {
                q.push_back(Point(ni, nj));
            } else if (getBoard(ni, nj) == EMPTY) return false;
        }
    }
    return true;
    
}

std::vector<Point> Board::get_next_legal_moves() {
    std::vector<Point> allowed_moves;
    for (int r = 1; r <= BSIZE; r++) {
        for (int c = 1; c < BSIZE; c++) {
            if (getBoard(r,c) == EMPTY) {
                if (isSuicide(r,c,player) && !canEat(r,c,player)) continue;
                allowed_moves.push_back(Point(r,c));
            }
        }
    }
    return allowed_moves;
}

