#include <stdio.h>
#include <chrono>
#include "Go.h"
#include "mcts.h"
#include "point.h"
#include "mpi.h"

#define NUM_MOVES 20
#define TIME_EACH_MOVE 10*1000 // ms

int main(int argc, char** argv) {
	Mcts* black;
	Mcts* white;
	Point p = Point(-1, -1);
	Board board;
	int step = 0;

	int num_procs, rank;
	int num_games = 0;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
	if(rank == 0){
		printf("hybrid start. black first\n");
	}

	auto start_time = std::chrono::steady_clock::now();

	while (step < NUM_MOVES) {
		black = new Mcts(TIME_EACH_MOVE, p);
		p = black->run(&board, rank, num_games);
		step++;
		if(rank == 0){
			printf("black : (%d,%d)\n", p.i, p.j);
		}
		
		board.update_board(p);

		if(rank == 0){
			board.print_board();
		}

		white = new Mcts(TIME_EACH_MOVE, p);
		p = white->run(&board, rank, num_games);
		step++;
		
		if(rank == 0){
			printf("white : (%d,%d)\n", p.i, p.j);
		}

		board.update_board(p);

		if(rank == 0){
			board.print_board();
		}

		delete white;
		delete black;
	}

	auto end_time = std::chrono::steady_clock::now();
	std::chrono::duration<double> diff = end_time - start_time;
	double seconds = diff.count();

	double fin_score = board.aga_score();

	if(rank == 0){
		printf("score:%.2f\n", fin_score);
		if (fin_score < 0) {
			printf("White wins!\n");
		} else if (fin_score > 0) {
			printf("Black wins!\n");
		} else {
			printf("Draw\n");
		}
		
		printf("%d games for %.2f seconds (%.2f GPS)\n", num_games, seconds, num_games / seconds);
	}


	MPI_Finalize();
}
