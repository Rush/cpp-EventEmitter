all: EventEmitter.hpp test benchmark example

EventEmitter.hpp: EventEmitter.sane.hpp compile.pl Makefile
	./compile.pl < EventEmitter.sane.hpp > EventEmitter.hpp

test: test.cpp EventEmitter.hpp EventEmitter.sane.hpp
	$(CXX) test.cpp -std=c++14 -o test -g -lpthread $(DEFS)

benchmark: benchmark.cpp EventEmitter.hpp
	$(CXX) benchmark.cpp -std=c++14 -o benchmark -g -lpthread -O3 $(DEFS)

example: example.cpp EventEmitter.hpp
	$(CXX) example.cpp -std=c++14 -o example $(DEFS)

clean:
	rm test EventEmitter.hpp