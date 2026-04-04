#include "Go.h"
#include "common.h"

bool Board::canEat(int i, int j, COLOR color) {
    setBoard(i, j, color);
    bool result = false;
    COLOR op_color = static_cast<COLOR>(color ^ 3);
    std::vector<bool> visited(BSIZE * BSIZE, false);
    std::deque<Point> q1;    

    for (int d = 0; d < 4; d++) {
        int ni = i + dir[d][0];
        int nj = j + dir[d][1];
        if (getBoard(ni, nj) == op_color && !visited[ni * BSIZE + nj]) {
            q1.push_back(Point(ni,nj));
            int liberty = 0;
            while (q1.size() >= 0) {
                Point f = q1.front();
                q1.pop_front();
                visited[f.i, f.j] = true;
                for (int dd = 0; dd < 4; dd++) {
                    int nni = f.i + dir[dd][0];
                    int nnj = f.j + dir[dd][1];
                    if (visited[nni * BSIZE + nnj]) continue;
                    if (getBoard(nni, nnj) == op_color) {
                        q1.push_back(Point(nni, nnj));
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
    
}

