# Compiler and flags
#CXX = g++
# this is the raspberry pi0 compiler
CXX = arm-linux-gnueabihf-g++
#CXX = $(CROSS_PREFIX)g++
CXXFLAGS = -Isrc/Drivers -Isrc/Tools -Isrc/ -Ilibs/bcm2835-1.71/src -Ilibs/pigpio-master -Wall -Wextra -pthread 
LDFLAGS = -Llibs/pigpio-master/build -lpigpio -lbcm2835 -lncurses

# Source files and target
SRCDIR = src
DRIVERDIR = src/Drivers
TOOLSDIR = src/Tools
BUILDDIR = build
SRCS = $(wildcard $(SRCDIR)/*.cpp) $(wildcard $(DRIVERDIR)/*.cpp) $(wildcard $(TOOLSDIR)/*.cpp) 
OBJS = $(patsubst %.cpp,$(BUILDDIR)/%.o,$(notdir $(SRCS)))
TARGET = pami

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(DRIVERDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(TOOLSDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(TARGET)

# Rebuild target
re: clean all

# Phony targets
.PHONY: all clean re
