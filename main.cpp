#include <stdio.h>
#include "Go.h"
#include "mcts.h"
#include "point.h"

#define NUM_MOVES 10
#define TIME_EACH_MOVE 2*60*1000 // ms

int main() {
	Mcts* black;
	Mcts* white;
	Point p;
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

    // TODO MAKE MORE ACCURATE SCORE FUNCTION FOR BOARD
	printf("score:%d\n", board.quick_score());
}
