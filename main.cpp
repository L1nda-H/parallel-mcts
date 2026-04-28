#include <stdio.h>
#include <chrono>
#include "Go.h"
#include "mcts.h"
#include "point.h"
#include "mpi.h"
#include <string>
#include <iostream>
#include <sstream>

#define TIME_EACH_MOVE 10*1000 // ms

int main(int argc, char** argv) {
	int num_procs, rank;
	int num_games = 0;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	Mcts* mcts;
	Point p = Point(-1, -1);
	Board* board;
	int step = 0;

	std::string command;
	bool running = true;
	bool root = rank == 0;
	int cmd_len = 0;
	double seconds = -1;

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
			int board_size;
			ss >> board_size;
			board = new Board(board_size);
			if (root) std::cout << "=\n\n";
		} else if (cmd_type == "clear_board")
		{
			board->clear(); 
            if (root) std::cout << "=\n\n";
		} else if (cmd_type == "komi")
		{
			if (root) std::cout << "=\n\n";
		} else if (cmd_type == "play")
		{
			std::string color, coord;
			ss >> color >> coord;
			
			Point p = Point (coord, board->get_bsize());
			board->update_board(p);
			if (root) std::cout << "=\n\n";
		} else if (cmd_type == "genmove")
		{
			std::string color;
            ss >> color;
            mcts = new Mcts(TIME_EACH_MOVE, Point(-1, -1));
			num_games = 0;

			auto start_time = std::chrono::steady_clock::now();
            p = mcts->run(board, rank, num_games);
			auto end_time = std::chrono::steady_clock::now();
			std::chrono::duration<double> diff = end_time - start_time;
			seconds = diff.count();

            board->update_board(p);

            if (root) {
				std::string gtp_coord;
                if (p.i == -1) {
                    gtp_coord = "= pass\n\n";
                } else {
                    gtp_coord = Point::pt_to_gtp(p, board->get_bsize());
                }
				double gps = num_games / seconds;
                char gps_msg[256];
                snprintf(gps_msg, sizeof(gps_msg), "# %d games for %.2f seconds (%.2f GPS)", num_games, seconds, gps);

                std::cout << "= " << gtp_coord << "\n" << gps_msg << "\n\n";
            }
            delete mcts;
		} else if (cmd_type == "final_score")
		{
			/* code */
		} else if (cmd_type == "showboard")
		{
			if (root) {
				std::cout << "=\n";
				board->print_board();
				std::cout << "\n";
			}
		} else {
			if (root) std::cout << "unknown command";
		}
	}

	double fin_score = board->aga_score();

	if(rank == 0){
		printf("score:%.2f\n", fin_score);
		if (fin_score < 0) {
			printf("White wins!\n");
		} else if (fin_score > 0) {
			printf("Black wins!\n");
		} else {
			printf("Draw\n");
		}
	}

	fflush(stdout);

	MPI_Finalize();
	return 0;
}
