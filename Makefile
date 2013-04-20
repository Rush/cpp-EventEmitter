all: test

test: test.cpp EventEmitter.hpp
	$(CXX) test.cpp -std=c++11 -o test -g