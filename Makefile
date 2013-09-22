all: EventEmitter.hpp test benchmark

EventEmitter.hpp: EventEmitter.sane.hpp compile.pl Makefile
	./compile.pl < EventEmitter.sane.hpp > EventEmitter.hpp

test: test.cpp EventEmitter.hpp EventEmitter.sane.hpp
	$(CXX) test.cpp -std=c++11 -o test -g -lpthread $(DEFS)

benchmark: benchmark.cpp EventEmitter.hpp
	$(CXX) benchmark.cpp -std=c++11 -o benchmark -g -lpthread -O3 -save-temps $(DEFS)
clean:
	rm test EventEmitter.hpp