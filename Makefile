# Compiler and flags
CXX = g++
CXXFLAGS = -I. -isystem /usr/include/eigen3 -Wall -Wextra -g

# Target executable
TARGET = main

# Source files
SRCS = main.cpp image.cpp stb_wrapper.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

# Rule to build object files
%.o: %.cpp
	$(CXX) -c $< $(CXXFLAGS)

# Clean up build files
clean:
	rm -f $(TARGET) $(OBJS)