import subprocess

def gtp_cmd(process, cmd):
    process.stdin.write(f"{cmd}\n".encode('utf-8'))
    process.stdin.flush()
    
    response = ""
    while True:
        # Read the engine standard output
        line = process.stdout.readline().decode('utf-8').strip()
        if line == "": 
            break
        response = line
    return response.replace("= ", "").strip()

def run_match(engine1_cmd, engine2_cmd, games=1):
    wins_e1 = 0
    wins_e2 = 0

    boardsize = 13
    
    print(f"Starting {games}-game benchmark...")

    with open("errors.log", "w") as my_log, open("pachi_debug.log", "w") as pachi_log:
    
        for i in range(games):
            # Alternate colors to prevent first-mover advantage
            e1_is_black = (i % 2 == 0)
            
            b_cmd = engine1_cmd if e1_is_black else engine2_cmd
            w_cmd = engine2_cmd if e1_is_black else engine1_cmd
            
            b_err = pachi_log if "pachi" in b_cmd else my_log
            w_err = pachi_log if "pachi" in w_cmd else my_log
            
            # Launch both executables
            black = subprocess.Popen(
                b_cmd, 
                stdin=subprocess.PIPE, 
                stdout=subprocess.PIPE, 
                stderr=b_err, 
                shell=True
            )
                
            white = subprocess.Popen(
                w_cmd, 
                stdin=subprocess.PIPE, 
                stdout=subprocess.PIPE, 
                stderr=w_err, 
                shell=True
            )
            
            # Initialize board
            gtp_cmd(black, f"boardsize {boardsize}")
            gtp_cmd(white, f"boardsize {boardsize}")
            gtp_cmd(black, "clear_board")
            gtp_cmd(white, "clear_board")
            
            passes = 0
            moves = 0
            
            # Play until both pass, or we hit a 300 move limit (prevents infinite loops)
            while passes < 2 and moves < 10:
                # Black move
                b_move = gtp_cmd(black, "genmove b")
                if b_move.lower() == "pass": passes += 1
                else: passes = 0
                
                gtp_cmd(white, f"play b {b_move}") # Tell White what Black did
                if passes == 2: break
                
                # White move
                w_move = gtp_cmd(white, "genmove w")
                if w_move.lower() == "pass": passes += 1
                else: passes = 0
                
                gtp_cmd(black, f"play w {w_move}") # Tell Black what White did
                moves += 2
        
            # ask whichever engine was Pachi to grade the final board.
            # We could also ask our own engine to grade the final board
            if "pachi" in b_cmd.lower():
                score_str = gtp_cmd(black, "final_score")
            else:
                score_str = gtp_cmd(white, "final_score")
                
            # Clean up the background processes
            black.kill()
            white.kill()
            
            # Parse the winner (e.g., "B+14.5" means Black won, "W+Resign" means White won)
            if "B+" in score_str:
                print(f"Game {i+1}: Black wins ({score_str})")
                if e1_is_black: wins_e1 += 1
                else: wins_e2 += 1
            elif "W+" in score_str:
                print(f"Game {i+1}: White wins ({score_str})")
                if not e1_is_black: wins_e1 += 1
                else: wins_e2 += 1
            else:
                print(f"Game {i+1}: Draw or unfinished")
            
    print("\n--- FINAL RESULTS ---")
    print(f"MCTS: {wins_e1} wins")
    print(f"Pachi:     {wins_e2} wins")

if __name__ == "__main__":
    # Point these directly to executables
    run_match("./build/go_mcts_parallel", "./pachi/pachi -t 10 threads=1,policy=ucb1,playout=light,prior=eqex=0,dynkomi=none,pondering=0,pass_all_alive", games=1)