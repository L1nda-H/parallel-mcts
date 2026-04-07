#include <stdio.h>
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
            while (!q.empty()) {
                Point f = q.front();
                q.pop_front();
                visited[f.i * BSIZEIDX + f.j] = true;
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
    while (!q.empty()) 
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

int Board::update_board(Point pos) {
    setBoard(pos.i, pos.j, player);

    COLOR op_color = static_cast<COLOR>(player ^ 3);
    std::vector<bool> visited(BSIZEIDX * BSIZEIDX, false);
    std::deque<Point> q1;
    std::deque<Point> q2;    

    int total = 0;
    for(int d = 0; d < 4; d++) {
        int ni = pos.i + dir[d][0];
        int nj = pos.j + dir[d][1];

        if (getBoard(ni, nj) == op_color && !visited[ni * BSIZEIDX + nj]) {
            int liberty = 0;
            q1.push_back(Point(ni, nj));
            q2.push_back(Point(q1.front()));
            while(q1.size() != 0) {
                Point f = q1.front();
                q1.pop_front();
                visited[ni * BSIZEIDX + nj] = true;
                for (int dd = 0; dd < 4; dd++) {
                    ni = f.i + dir[dd][0];
                    nj = f.j + dir[dd][1];
                    if (visited[ni * BSIZEIDX + nj])continue;
					if (getBoard(ni, nj) == op_color) {
						Point tp = Point(ni, nj);
						q1.push_back(tp);
						q2.push_back(tp);
					} else if (getBoard(ni, nj) == EMPTY) {
						liberty++;
					}
                }
            }
            if (liberty == 0) {
				total += q2.size();
				for (std::deque<Point>::iterator it = q2.begin(); it != q2.end(); it++) {
					Point p = *it;
					setBoard(p.i, p.j, EMPTY);
				}
			}
			q2.clear();
        }
    }
    player = op_color;
    return total;
}

void Board::print_board() {
    for (int i = 0; i < BSIZE + 1; i++) {
		printf("=");
	}
	printf("\n");
	for (int i = 1; i < BSIZE + 1 ; i++) {
		for (int j = 1; j < BSIZE + 1; j++) {
			if (getBoard(i, j) == WHITE) {
				printf("W");
			} else if (getBoard(i, j) == BLACK) {
				printf("B");
			} else {
				printf(".");
			}
		}
		printf("\n");
	}
	for (int i = 0; i < BSIZE + 1; i++) {
		printf("=");
	}
	printf("\n");
}

int Board::quick_score() {
    int black = 0;
	int white = 0;

	for (int i = 1; i < BSIZE + 1; i++) {
		for (int j = 1; j < BSIZE + 1; j++) {
			if (getBoard(i, j) == WHITE) {
				white++;
			} else if (getBoard(i, j) == BLACK) {
				black++;
			}
		}
	}

	return black - white;
}
