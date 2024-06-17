# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -I src/ -I src/Drivers/ -I src/Tools/ -Wall -O2

# Libraries
LIBS = -lpigpio -lpthread -lncurses -lm

# Directories
SRC_DIR = src
DRIVERS_DIR = src/Drivers
TOOLS_DIR = src/Tools

# Source files
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp) \
            $(wildcard $(DRIVERS_DIR)/*.cpp) \
            $(wildcard $(TOOLS_DIR)/*.cpp)

# Object files
OBJ_FILES = $(SRC_FILES:.cpp=.o)

# Output executable
TARGET = pami

# Default rule
all: $(TARGET)

# Rule to link the final executable
$(TARGET): $(OBJ_FILES)
	$(CXX) -o $@ $^ $(LIBS)

# Rule to compile .cpp files to .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJ_FILES) $(TARGET)

# Rebuild rule
re: clean all

# Phony targets
.PHONY: all clean re
