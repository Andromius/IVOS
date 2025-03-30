# Compiler
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Icore -Ilogging -Ihttp
LDFLAGS = -lssl -lcrypto

# Directories
SRC_DIRS = core logging http
OBJ_DIR = build
BIN = http_server

# Find all source files
SRCS = $(wildcard $(addsuffix /*.cpp, $(SRC_DIRS))) main.cpp
OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# Default target
all: $(BIN)

# Create build directory structure
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link executable
$(BIN): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(BIN)

# Phony targets
.PHONY: all clean
