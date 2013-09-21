all: test

EventEmitter.hpp: EventEmitter.sane.hpp
	./compile.pl < EventEmitter.sane.hpp > EventEmitter.hpp
test: test.cpp EventEmitter.hpp
	$(CXX) test.cpp -std=c++11 -o test -g -lpthread

clean:
	rm test EventEmitter.hpp