# Compiler
CXX = g++
CXXFLAGS = -Wall -std=c++17

# Target executables
TARGETS = main_program store_program product_program

# Source files for each target
MAIN_SRCS = main.cpp logger.cpp
STORE_SRCS = store.cpp logger.cpp
PRODUCT_SRCS = product.cpp logger.cpp

# Object files
MAIN_OBJS = $(MAIN_SRCS:.cpp=.o)
STORE_OBJS = $(STORE_SRCS:.cpp=.o)
PRODUCT_OBJS = $(PRODUCT_SRCS:.cpp=.o)

# Default target
all: $(TARGETS)

# Link object files to create the executables
main_program: $(MAIN_OBJS)
	$(CXX) $(CXXFLAGS) -o main_program $(MAIN_OBJS)

store_program: $(STORE_OBJS)
	$(CXX) $(CXXFLAGS) -o store_program $(STORE_OBJS)

product_program: $(PRODUCT_OBJS)
	$(CXX) $(CXXFLAGS) -o product_program $(PRODUCT_OBJS)

# Compile source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(TARGETS) $(MAIN_OBJS) $(STORE_OBJS) $(PRODUCT_OBJS)

# Phony targets
.PHONY: all clean


