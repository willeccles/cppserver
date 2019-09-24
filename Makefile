all:
	$(CXX) $(wildcard *.cpp) -o server -std=c++2a -O3
