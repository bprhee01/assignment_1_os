CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2

all: my-sum

my-sum: my-sum.cpp
	$(CXX) $(CXXFLAGS) -o my-sum my-sum.cpp

clean:
	rm -f my-sum

.PHONY: all clean