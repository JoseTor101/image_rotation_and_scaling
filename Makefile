# Compiler and flags
CXX = g++
CXXFLAGS = -I. -isystem /usr/include/eigen3 -Wall -Wextra -g

# Target executables
TARGET = main
BENCHMARK = benchmark

# Source files
SRCS = main.cpp image.cpp stb_wrapper.cpp
BENCHMARK_SRCS = benchmark.cpp image.cpp stb_wrapper.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)
BENCHMARK_OBJS = $(BENCHMARK_SRCS:.cpp=.o)

# Default target
all: $(TARGET) $(BENCHMARK)

# Build the target
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

# Build the benchmark
$(BENCHMARK): $(BENCHMARK_OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

# Rule to build object files
%.o: %.cpp
	$(CXX) -c $< $(CXXFLAGS)

# Clean up build files
clean:
	rm -f $(TARGET) $(BENCHMARK) $(OBJS) $(BENCHMARK_OBJS)