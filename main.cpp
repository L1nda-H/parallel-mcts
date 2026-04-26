#include <stdio.h>
#include <chrono>
#include "Go.h"
#include "mcts.h"
#include "point.h"
#include "mpi.h"
#include <string>
#include <iostream>
#include <sstream>

#define NUM_MOVES 20
#define TIME_EACH_MOVE 10*1000 // ms

int main(int argc, char** argv) {
	int num_procs, rank;
	int num_games = 0;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	Mcts* black;
	Mcts* white;
	Point p = Point(-1, -1);
	Board board;
	int step = 0;

	std::string command;
	bool running = true;
	bool root = rank == 0;
	int cmd_len = 0;

	while (running) {
		if (root) {
			std::getline(std::cin, command);
			cmd_len = command.length();
		}
		MPI_Bcast(&cmd_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
		if (!root){
			command.resize(cmd_len);
		}
		MPI_Bcast(&command[0], cmd_len, MPI_CHAR, 0, MPI_COMM_WORLD);
		
		std::stringstream ss(command);
		std::string cmd_type;
		ss >> cmd_type;

		if (cmd_type == "protocol_version") 
		{
			if (rank == 0) std::cout << "=\n\n";
		} else if (cmd_type == "name")
		{
			if (root) std::cout << "= ParallelMCTS\n\n";
		} else if (cmd_type == "version")
		{
			if (root) std::cout << "=\n\n";
		} else if (cmd_type == "list_commands")
		{
			if (root) std::cout << "= protocol_version\nname\nversion\nlist_commands\nquit\nboardsize\nclear_board\nkomi\nplay\ngenmove\nfinal_score\nshowboard\n\n";
		} else if (cmd_type == "quit") {
			running = false;
			if (root) std::cout << "=\n\n";
		} else if (cmd_type == "boardsize")
		{
			if (root) std::cout << "= " << BSIZE << "\n\n";
		} else if (cmd_type == "clear_board")
		{
			board = Board(); 
            if (root) std::cout << "=\n\n";
		} else if (cmd_type == "komi")
		{
			if (root) std::cout << "= " << KOMI << "\n\n";
		} else if (cmd_type == "play")
		{
			std::string color, coord;
			ss >> color >> coord;
			
			Point p = Point (coord);
			board.update_board(p);
		} else if (cmd_type == "genmove")
		{
			/* code */
		} else if (cmd_type == "final_score")
		{
			/* code */
		} else if (cmd_type == "showboard")
		{
			/* code */
		} else {
			if (root) std::cout << "unknown command";
		}
	}

    
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
