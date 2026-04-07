#include <stdio.h>
#include "Go.h"
#include "mcts.h"
#include "point.h"

#define NUM_MOVES 20
#define TIME_EACH_MOVE 10*1000 // ms

int main() {
	Mcts* black;
	Mcts* white;
	Point p = Point(-1, -1);
	Board board;
	int step = 0;
    
	printf("hybrid start. black first\n");

	while (step < NUM_MOVES) {
		black = new Mcts(TIME_EACH_MOVE, p);
		p = black->run(&board);
		step++;
		printf("black : (%d,%d)\n", p.i, p.j);
		
		board.update_board(p);
		board.print_board();
		white = new Mcts(TIME_EACH_MOVE, p);
		p = white->run(&board);
		step++;
		
		printf("white : (%d,%d)\n", p.i, p.j);
		board.update_board(p);
		board.print_board();
		delete white;
		delete black;
	}

	double fin_score = board.aga_score();

	printf("score:%.2f\n", fin_score);
	if (fin_score < 0) {
		printf("White wins!\n");
	} else if (fin_score > 0) {
		printf("Black wins!\n");
	} else {
		printf("Draw\n");
	}
}
