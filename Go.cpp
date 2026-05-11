#include <stdio.h>
#include <iostream>
#include "Go.h"
#include "tdeque.h"

bool Board::canEat(int i, int j, COLOR color) {
    setBoard(i, j, color);
    bool result = false;
    COLOR op_color = static_cast<COLOR>(color ^ 3);
    thread_local TDeque q; 
    thread_local bool visited[MAX_POINTS];

    q.clear(); 
    std::memset(visited, false, sizeof(visited));

    for (int d = 0; d < 4; d++) {
        int ni = i + dir[d][0];
        int nj = j + dir[d][1];
        
        if (getBoard(ni, nj) == op_color && !visited[ni * bsize_idx + nj]) {
            q.clear();
            q.push_back(Point(ni,nj));
            visited[ni * bsize_idx + nj] = true;
            
            int liberty = 0;
            while (!q.empty()) {
                Point f = q.front();
                q.pop_front();
                for (int dd = 0; dd < 4; dd++) {
                    int nni = f.i + dir[dd][0];
                    int nnj = f.j + dir[dd][1];
                    if (visited[nni * bsize_idx + nnj]) continue;
                    if (getBoard(nni, nnj) == op_color) {
                        visited[nni * bsize_idx + nnj] = true;
                        q.push_back(Point(nni, nnj));
                    } else if (getBoard(nni, nnj) == EMPTY) {
                        liberty++;
                        break;
                    }
                }
                if (liberty > 0) {
                    break;
                }
            }
            if (liberty == 0) {
                result = true;
                break;
            }
        }
    }
    
    setBoard(i, j, COLOR::EMPTY);
    return result;
}

bool Board::isSuicide(int i, int j, COLOR color) {
    thread_local TDeque q;
    thread_local bool visited[MAX_POINTS];

    q.clear();
    std::memset(visited, false, sizeof(visited));

    q.push_back(Point(i, j));
    visited[i * bsize_idx + j] = true;
    while (!q.empty()) 
    {
        Point f = q.front();
        q.pop_front();
        for (int d = 0; d < 4; d++) {
            int ni = f.i + dir[d][0];
            int nj = f.j + dir[d][1];
            if (visited[ni * bsize_idx + nj]) continue;
            if (getBoard(ni, nj) == color) {  
                visited[ni * bsize_idx + nj] = true;
                q.push_back(Point(ni, nj));
            } else if (getBoard(ni, nj) == EMPTY) return false;
        }
    }
    return true;
    
}

int Board::countLiberties(int i, int j, COLOR color) {
    thread_local TDeque q;
    thread_local bool visited[MAX_POINTS];
    thread_local bool visited_liberty[MAX_POINTS];

    q.clear();
    std::memset(visited, false, sizeof(visited));
    std::memset(visited_liberty, false, sizeof(visited_liberty));

    q.push_back(Point(i, j));
    visited[i * bsize_idx + j] = true;

    int liberties = 0;

    while (!q.empty() && liberties < 3) 
    {
        Point f = q.front();
        q.pop_front();
        for (int d = 0; d < 4; d++) {
            int ni = f.i + dir[d][0];
            int nj = f.j + dir[d][1];
            int idx = ni * bsize_idx + nj;

            if (visited[idx]) continue;

            if (getBoard(ni, nj) == color) {  
                visited[ni * bsize_idx + nj] = true;
                q.push_back(Point(ni, nj));
            } else if (getBoard(ni, nj) == EMPTY) {
                if (!visited_liberty[idx]) {
                    visited_liberty[idx] = true;
                    liberties++;
                }
            }
        }
    }
    return liberties;
}

std::vector<Point> Board::get_next_legal_moves() {
    std::vector<Point> allowed_moves;
    for (int r = 1; r <= bsize; r++) {
        for (int c = 1; c <= bsize; c++) {
            if (getBoard(r,c) == EMPTY) {
                if (countLiberties(r,c,player) < 2 && !canEat(r,c,player)) continue;
                allowed_moves.push_back(Point(r,c));
            }
        }
    }
    return allowed_moves;
}

int Board::update_board(Point pos) {
    COLOR curr_play = player;
    COLOR op_color = static_cast<COLOR>(player ^ 3);
    player = op_color;
    if (pos.i == -1 || pos.j == -1) {
        return 0;
    }
    
    setBoard(pos.i, pos.j, curr_play);
    state_hash = hash_table.updateHash(state_hash, Point::point_to_id(pos, bsize), curr_play);

    thread_local bool visited[MAX_POINTS];

    std::memset(visited, false, sizeof(visited));

    thread_local TDeque q1;
    thread_local TDeque q2;
    
    q1.clear();
    q2.clear();

    int total = 0;
    for(int d = 0; d < 4; d++) {
        int ni = pos.i + dir[d][0];
        int nj = pos.j + dir[d][1];

        if (getBoard(ni, nj) == op_color && !visited[ni * bsize_idx + nj]) {
            int liberty = 0;
            q1.push_back(Point(ni, nj));
            visited[ni * bsize_idx + nj] = true;
            q2.push_back(Point(q1.front()));
            while(!q1.empty()) {
                Point f = q1.front();
                q1.pop_front();
                for (int dd = 0; dd < 4; dd++) {
                    ni = f.i + dir[dd][0];
                    nj = f.j + dir[dd][1];
                    if (visited[ni * bsize_idx + nj])continue;
					if (getBoard(ni, nj) == op_color) {
						Point tp = Point(ni, nj);
                        visited[ni * bsize_idx + nj] = true;
						q1.push_back(tp);
						q2.push_back(tp);
					} else if (getBoard(ni, nj) == EMPTY) {
						liberty++;
					}
                }
            }
            if (liberty == 0) {
				total += q2.size();
				for (int it = q2.head; it != q2.tail; it++) {
					Point p = q2.get(it);
                    if (zobrist) {
                        int pos = Point::point_to_id(p, bsize);
                        state_hash = hash_table.updateHash(state_hash, pos, getBoard(p.i, p.j));
                    }
					setBoard(p.i, p.j, COLOR::EMPTY);
				}
			}
			q2.clear();
        }
    }
    return total;
}

void Board::print_board() {
    for (int i = 0; i < bsize + 1; i++) {
        std::cout << "=";
    }
    std::cout << "\n";
    
    for (int i = 1; i < bsize + 1 ; i++) {
        for (int j = 1; j < bsize + 1; j++) {
            if (getBoard(i, j) == WHITE) {
                std::cout << "O ";
            } else if (getBoard(i, j) == BLACK) {
                std::cout << "X ";
            } else {
                std::cout << ". ";
            }
        }
        std::cout << "\n";
    }
    
    for (int i = 0; i < bsize + 1; i++) {
        std::cout << "=";
    }
    std::cout << "\n";
}

int Board::quick_score() {
    int black_score = 0;
    int white_score = 0;
    
    thread_local bool visited[MAX_POINTS];
    std::memset(visited, false, sizeof(bool));

    for (int i = 1; i <= bsize; i++) {
        for (int j = 1; j <= bsize; j++) {
            int idx = i * bsize_idx + j;
            COLOR color = static_cast<COLOR>(getBoard(i, j)); 
            
            if (color == BLACK) {
                black_score++;
            } 
            else if (color == WHITE) {
                white_score++;
            } 
            else if (color == EMPTY && !visited[idx]) {
                int empty_count = 0;
                bool touches_black = false;
                bool touches_white = false;
                
                std::deque<Point> q;
                q.push_back(Point(i, j));
                visited[idx] = 1;
                
                while (!q.empty()) {
                    Point curr = q.front();
                    q.pop_front();
                    empty_count++;
                    
                    for (int d = 0; d < 4; d++) {
                        int ni = curr.i + dir[d][0];
                        int nj = curr.j + dir[d][1];
                        int nidx = ni * bsize_idx + nj;
                        
                        COLOR ncolor = static_cast<COLOR>(getBoard(ni, nj));
                        
                        if (ncolor == BLACK) {
                            touches_black = true;
                        } else if (ncolor == WHITE) {
                            touches_white = true;
                        } else if (ncolor == EMPTY && !visited[nidx]) {
                            visited[nidx] = 1;
                            q.push_back(Point(ni, nj));
                        }
                    }
                }
                
                if (touches_black && !touches_white) {
                    black_score += empty_count;
                } else if (touches_white && !touches_black) {
                    white_score += empty_count;
                }
            }
        }
    }

    return (black_score) - (white_score + KOMI); 
}
