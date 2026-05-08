import subprocess

MOVE_LIMIT = 10

def gtp_cmd(process, cmd):
    process.stdin.write(f"{cmd}\n".encode('utf-8'))
    process.stdin.flush()
    
    lines = []
    while True:
        line = process.stdout.readline().decode('utf-8')
        
        if line == "\n" or line == "\r\n": 
            break
            
        lines.append(line.rstrip('\r\n'))
        
    response = "\n".join(lines)
    if response.startswith("= "):
        return response[2:]
    elif response.startswith("="):
        return response[1:].lstrip('\n')
        
    return response.strip()

def run_match(engine1_cmd, engine2_cmd, games=1):
    wins_e1 = 0
    wins_e2 = 0
    boardsize = 13
    gps_record =  ["\n"] * MOVE_LIMIT
    
    with open("game_results.txt", "w") as out_log, open("errors.log", "w") as my_log, open("pachi_debug.log", "w") as pachi_log, open("gps.txt","w") as gps:
        
        def log_print(msg):
            print(msg)
            out_log.write(msg + "\n")
            out_log.flush()

        log_print(f"Starting {games}-game benchmark...")

        for i in range(games):
            log_print(f"\n=======================")
            log_print(f"--- Game {i+1} ---")
            
            # Alternate colors to prevent first-mover advantage
            e1_is_black = (i % 2 == 0)
            
            b_cmd = engine1_cmd if e1_is_black else engine2_cmd
            w_cmd = engine2_cmd if e1_is_black else engine1_cmd
            
            b_name = "Parallel MCTS" if e1_is_black else "Pachi"
            w_name = "Pachi" if e1_is_black else "Parallel MCTS"
            
            log_print(f"Black: {b_name}")
            log_print(f"White: {w_name}\n")
            
            b_err = pachi_log if "pachi" in b_cmd else my_log
            w_err = pachi_log if "pachi" in w_cmd else my_log
            
            try:
                # Launch both executables
                black = subprocess.Popen(b_cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=b_err, shell=True)
                white = subprocess.Popen(w_cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=w_err, shell=True)
                
                # Initialize board
                gtp_cmd(black, f"boardsize {boardsize}")
                gtp_cmd(white, f"boardsize {boardsize}")
                gtp_cmd(black, "clear_board")
                gtp_cmd(white, "clear_board")
                
                passes = 0
                moves = 0
                
                # Play until both pass, or we hit a 150 move limit (prevents infinite loops)
                while passes < 2 and moves < MOVE_LIMIT:
                    # --- Black move ---
                    b_raw = gtp_cmd(black, "genmove b")
                    b_parts = b_raw.split('\n')
                    b_move = b_parts[0].strip()
                    
                    log_print(f"Black ({b_name}) plays: {b_move}")
                    
                    for line in b_parts[1:]:
                        if line.startswith("#"):
                            log_print(f"    {line}")
                            gps_record[moves] += (f"{line}")
                    
                    if b_move.lower() == "pass": passes += 1
                    elif b_move.lower() == "resign": passes = 3
                    else: passes = 0
                    
                    gtp_cmd(white, f"play b {b_move}") # Tell White what Black did
                    if passes == 2: break
                    
                    # --- White move ---
                    w_raw = gtp_cmd(white, "genmove w")
                    w_parts = w_raw.split('\n')
                    w_move = w_parts[0].strip()
                    
                    log_print(f"White ({w_name}) plays: {w_move}")
                    
                    for line in w_parts[1:]:
                        if line.startswith("#"):
                            log_print(f"    {line}")
                            gps_record[moves] += (f"{line}")
                    
                    if w_move.lower() == "pass": passes += 1
                    elif w_move.lower() == "resign": passes = 3
                    else: passes = 0
                    
                    gtp_cmd(black, f"play w {w_move}") # Tell Black what White did
                    moves += 1

                    curr_board = gtp_cmd(white if w_name == "Pachi" else black, "showboard")
                    
                    log_print(curr_board)

                    # curr_board = gtp_cmd(black if w_name == "Pachi" else white, "showboard")
                    
                    # log_print(curr_board)
            
                # Retrieve and log the final board state
                log_print("\n--- Final Board State ---")
                final_board = gtp_cmd(white if w_name == "Pachi" else black, "showboard")
                log_print(final_board)
                log_print("") # Add an empty line for spacing
                
                # Score the board
                if "pachi" in b_cmd.lower():
                    score_str = gtp_cmd(black, "final_score")
                else:
                    score_str = gtp_cmd(white, "final_score")
                
                # Parse the winner
                if "B+" in score_str:
                    log_print(f"Game {i+1} Result: Black wins ({score_str})")
                    if e1_is_black: wins_e1 += 1
                    else: wins_e2 += 1
                elif "W+" in score_str:
                    log_print(f"Game {i+1} Result: White wins ({score_str})")
                    if not e1_is_black: wins_e1 += 1
                    else: wins_e2 += 1
                else:
                    log_print(f"Game {i+1} Result: Draw or unfinished")
            except KeyboardInterrupt:
                break
            finally:
                black.kill()
                white.kill()
        
        gps.writelines(gps_record)
        log_print("\n--- FINAL RESULTS ---")
        log_print(f"Your MCTS: {wins_e1} wins")
        log_print(f"Pachi:     {wins_e2} wins")

if __name__ == "__main__":
    # Point these directly to executables
    run_match("srun -n 2 ./build/go_mcts_parallel", "./pachi/pachi -t 1 -d0 -o pachilog.log threads=1,policy=ucb1,playout=light,prior=eqex=0,dynkomi=none,pondering=0,pass_all_alive", games=2)