#include "EventEmitter.hpp"

#include <cstdio>
#include <string>
#include <cassert>
#include <thread>
#include <chrono>
#include <iostream>
#include <map>

EventProviderThreaded(Test, (int, int, std::string))


EventProviderDeferred(ThreadSignal, (std::thread::id, int))
EventProviderDeferred(Test1, (int, int, std::string))
EventProviderDeferred(Test2, (std::string))

//EventProvider(ThreadSignal, (std::thread::id))
class Test2 : public Test1DeferredEventProvider, public Test2DeferredEventProvider, public ThreadSignalDeferredEventProvider, public TestThreadedEventProvider {
	int field2;
public:
};


class Test1 : public Test1DeferredEventProvider
{
public:
};



#include <exception>
class test_exception: public std::exception
{
	std::string msg;
public:
	test_exception(const char* _msg) : msg(_msg) {}
	virtual const char* what() const throw() {
		return msg.c_str();
	}
};

template <typename X, typename A>
inline void Assert(A assertion, const char* msg = "")
{
    if( !assertion ) throw X(msg);
}

#undef assert
#define assert Assert<test_exception>

int main()
{
	try {
		Test2 test;
		
		printf("sizeof %i\n", sizeof(TestThreadedEventProvider));
		
		int runs1 = 0, runs2 = 0, runs3 = 0;
		
		test.onTest1([=,&runs1](int a, int b, std::string str) {
			runs1++;
			printf("Handler1 %i, %i, %s\n", a, b, str.c_str());
		});
		
		test.onceTest1([=,&runs2](int a, int b, std::string str) {
			runs2++;
			printf("Handler2, %i, %i, %s\n", a, b, str.c_str());
		});
		
		
		
		test.triggerTest1(1, 2, "A");
		
		test.triggerTest1(3, 4, "B");
		
		assert(test.runDeferred() == true); // still more tasks
		assert(runs1 == 1);	
		assert(runs2 == 1);
		assert(test.runDeferred() == false); // false, no more to run
		assert(runs1 == 2);	
		assert(runs2 == 1);
		assert(test.runDeferred() == false);
		assert(runs1 == 2);	
		assert(runs2 == 1);
		
		test.removeAllTest1Handlers();
		auto future = test.futureOnceTest();
		test.triggerTest(213, 999, "B");
		auto t = future.get();
		
		assert(std::get<0>(t) == 213);
		assert(std::get<1>(t) == 999);
		assert(std::get<2>(t) == "B");
		
		test.triggerTest1(12, 123, "A");
		
		auto f = std::async(std::launch::async, [=,&test]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			std::cout << "Trigger ASYNC\n";
			test.triggerTest(10, 20, "ASYNC");
		});
		test.waitTest([=](int a, int b, std::string str) {
			assert(str == "ASYNC");
		});
		std::cout << "Stop waiting #1\n";
		
		
		f = std::async(std::launch::async, [=,&test]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			std::cout << "Triggering second test..\n";
			test.triggerTest(10, 20, "ASYNC");
		});
		std::cout << "Start waiting #2\n";
		bool status = test.waitTest([=](int a, int b, std::string str) {
			assert(false);
		}, std::chrono::milliseconds(500));
		std::cout << "Stop waiting " << status << "\n";
		
		
		
		assert(test.runDeferred() == false); // nothing to run, all handlers removed
		assert(runs1 == 2);	// removed all handlers
		assert(runs2 == 1);
		
		test.onTest1([=,&runs3](int a, int b, std::string str) {
			runs3++;
			printf("Handler1 %i, %i, %s\n", a, b, str.c_str());
		});
		
		test.Test1EventProvider::triggerTest1(5, 6, "C");
		assert(runs3 == 1);
		
		
		volatile int threadNum = 0;
		bool async = false;
		test.asyncOnceTest([=,&async](int, int, std::string str) {
			async = true;
		});
		
		test.triggerTest(43, 324, "dupa");
		test.runDeferred();
		
		volatile bool doExit = false;
		auto threadFunc = [=, &doExit, &test,&threadNum]() {
			
			threadNum++;
			int counter = 0;
			while(!doExit) {
				test.triggerThreadSignal(std::this_thread::get_id(), counter++);
			}
		};
		std::thread thr1(threadFunc), thr2(threadFunc);
		
		
		std::map<std::thread::id, int> counters;
		test.onThreadSignal([=,&counters](std::thread::id id, int counter) {
			counters[id] = counter;
		});
		for(int i = 0;i < 100 || counters.size() < 2;) {
			if(test.runDeferred())
				i++;
		}
		doExit = true;
		
		//	while(threadNum < 2 || counters.size() < 2);
		thr1.join();
		thr2.join();	
		
		assert(counters.size() == 2);
		
		assert(async == true);	
		
		test.clearDeferred();
		
		printf("ALL IS GOOD\n");
	}
	catch(std::string exception) {
		std::cout << "Got exception\n";
	}
	
	
	return 0;
}

