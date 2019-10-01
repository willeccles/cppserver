all: server threadpooltest

server:
	$(CXX) main.cpp -o server -std=c++2a -O3

threadpooltest:
	$(CXX) threadpool.cpp testthreadpool.cpp -o tptest -std=c++2a -O3
