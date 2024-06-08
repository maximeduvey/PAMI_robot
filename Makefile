# Compiler and flags
CXX = g++
CXXFLAGS = -Ilibs/pigpio-master -Wall -Wextra -pthread -lncurses -lm -Isrc -Ilibs/bcm2835-1.71/src
LDFLAGS = -Llibs/pigpio-master/build -lpigpio -lbcm2835

# Source files and target
SRCDIR = src
DRIVERDIR = src/Drivers
SRC = $(wildcard $(SRCDIR)/*.cpp) $(wildcard $(DRIVERDIR)/*.cpp)
TARGET = pami

# Default target
all: $(TARGET)

# Link
$(TARGET): $(SRC)
	$(CXX) $(SRC) -o $(TARGET) $(CXXFLAGS) $(LDFLAGS)

# Clean target
clean:
	rm $(TARGET)

# Rebuild target
re: clean all

# Phony targets
.PHONY: all clean re
