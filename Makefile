# The C++ compiler wrapper on Perlmutter
CXX = CC

# --- NORMAL FLAGS (For Speed) ---
CXXFLAGS = -O3 -Wall -std=c++11

# --- DEBUG FLAGS (For Bug Hunting) ---
# -g : Adds debugging symbols (so you see exact file/line numbers)
# -O0 : Turns off optimizations (so the code execution matches your source perfectly)
# -fsanitize=address : Injects memory checks to catch double-frees, out-of-bounds, and leaks
DEBUGFLAGS = -g -O0 -Wall -std=c++11 -fsanitize=address

# The name of your final executable
TARGET = go_mcts

SRCS = main.cpp Go.cpp mcts_serial.cpp
OBJS = $(SRCS:.cpp=.o)

# The default rule: build the optimized target
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Normal object compilation
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- THE NEW DEBUG TARGET ---
# When you type 'make debug', it recompiles everything with AddressSanitizer
debug: clean
	$(CXX) $(DEBUGFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(OBJS) $(TARGET) game_output.txt
