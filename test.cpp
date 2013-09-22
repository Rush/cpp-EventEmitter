#include "EventEmitter.sane.hpp"

#include <exception>
#include <iostream>

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

void runTest(std::function<void()> test, const std::string& str) {
	try {
		test();
		std::cout << "[ OK ] " << str << "\n";
	} catch(test_exception exception) {
		std::cout << "[Fail] " << str << "\n";
		std::cerr << "===========\n" << exception.what() << "\n===========\n";
	}
}


int main()
{
	
	runTest([] {
		int counter1 = 0, counter2 = 0;
		ExampleEventProvider test;
		test.onExample([&counter1](int a, int b, std::string str) {
			counter1 += a + b;
		});
		
		test.onceExample([&counter2](int a, int b, std::string str) {
			counter2 += a + b;
		});
		
		test.triggerExample(1, 5, "A");
		
		assert(counter1 == 6, "on: should equal sum of arguments");
		assert(counter2 == 6, "once: should equal sum of arguments");
		
		test.triggerExample(3, 7, "B");
		assert(counter1 == 16, "on: second trigger should fire");
		assert(counter2 == 6, "once: second trigger should not fire");
		
	}, "EventProvider - on, once, trigger");
	
	runTest([] {
		ExampleEventProvider test;
		int sum = 0;
		auto lambda = [&](int a, int b, std::string str) {
			sum += a + b;
		};
		auto handler = test.onExample(lambda);
		test.triggerExample(1, 3, "A");
		test.triggerExample(5, 7, "B");
		test.removeExampleHandler(handler);
		test.triggerExample(11, 13, "B");
		
		assert(sum == 16, "removeExampleHandler: third trigger should not run");
		
		handler = test.onceExample(lambda);
		test.removeExampleHandler(handler);
		test.triggerExample(11, 13, "B");
		assert(sum == 16, "removeExampleHandler: once handler should have been removed");			
	}, "EventProvider - removeHandler");
	
	
	runTest([] {
		ExampleEventProvider test;
		int sum = 0;
		auto lambda = [&](int a, int b, std::string str) {
			sum += a + b;
		};
		test.onExample(lambda);
		test.onExample(lambda);
		
		test.triggerExample(1, 3, "A");
		test.triggerExample(5, 7, "B");
		assert(sum == 32, "multiple handlers should have been added");
		test.removeAllExampleHandlers();
		assert(sum == 32, "all handlers should have been removed");
	}, "EventProvider - removeAllHandlers");
	
	
	// TODO: make this work!!
	// 		runTest([] {
	// 			ExampleEventProvider test;
	// 			
	// 			int sum = 0;
	// 			ExampleEventProvider::HandlerPtr ptr =  test.onExample([&](int a, int b, std::string str) {
	// 				sum += a;
	// 				test.removeExampleHandler(ptr);
	// 			});
	// 			test.triggerExample(11, 13, "B");
	// 			assert(sum == 11, "removeExampleHandler: third trigger should not run");
	// 			assert(sum == 16, "removeExampleHandler: once handler should have been removed");			
	// 		}, "EventProvider - removeHandler from within self");
	runTest([] {
		int counter1 = 0, counter2 = 0;
		ExampleDeferredEventProvider test;
		test.onExample([&counter1](int a, int b, std::string str) {
			counter1 += a + b;
		});
		
		test.onceExample([&counter2](int a, int b, std::string str) {
			counter2 += a + b;
		});
		
		test.triggerExample(1, 5, "A");
		test.triggerExample(3, 7, "B");
		
		assert(counter1 == 0, "on: should not have been called at all");
		assert(counter2 == 0, "on: should not have been called at all");
		test.runDeferred();
		
		assert(counter1 == 6, "on: should equal sum of arguments");
		assert(counter2 == 6, "once: should equal sum of arguments");
		test.runDeferred();
		
		assert(counter1 == 16, "on: second trigger should fire");
		assert(counter2 == 6, "once: second trigger should not fire");
		
		test.removeAllExampleHandlers();
		test.runAllDeferred();
		assert(counter1 == 16, "removeAllHandlers should have removed handler");
		
	}, "EventDeferredProvider - on, once, trigger, removeAllHandlers");
	
	
	runTest([]{
		ExampleThreadedEventProvider test;
		auto future = test.futureOnceExample();
		test.triggerExample(213, 999, "B");
		auto t = future.get();
		assert(std::get<0>(t) == 213, "Should got 1st argument");
		assert(std::get<1>(t) == 999, "Should got 2nd argument");
		assert(std::get<2>(t) == "B", "Should got 3rd argument");
			
	}, "EventThreadedProvider - futureOnce");
	
	
	int runs1 = 0, runs2 = 0, runs3 = 0;
	
	
	runTest([]{
		ExampleThreadedEventProvider test;
		auto f = std::async(std::launch::async, [=,&test]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			test.triggerExample(10, 20, "ASYNC");
		});
		int ares, bres;
		std::string strres;
		std::thread::id id;
		test.waitExample([&](int a, int b, std::string str) {
			ares = a;
			bres = b;
			strres = str;
			id = std::this_thread::get_id();
		});
		assert(id != std::this_thread::get_id(), "Should have run in another thread");
		assert(ares == 10 && bres == 20 && strres == "ASYNC", "All values should match");
	}, "EventThreadedProvider - wait for trigger in std::async");
		
	runTest([]{
		ExampleThreadedEventProvider test;
		auto f = std::async(std::launch::async, [=,&test]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			test.triggerExample(10, 20, "ASYNC");
		});
		bool result = false;
		bool status = test.waitExample([&result](int a, int b, std::string str) {
			result = true;
		}, std::chrono::milliseconds(50));
		assert(result == false && status == false, "Should have timed out");
	}, "EventThreadedProvider - wait for trigger in std::async with timeout");
	
	runTest([]{
		ExampleThreadedEventProvider test;
		bool async = false;
		std::thread::id id;
		test.asyncOnceExample([&](int, int, std::string str) {
			id = std::this_thread::get_id();
			async = true;
		});
		
		test.deferExample(0, 0, "");
		test.runAllDeferred();
		while(!async) {}
		assert(id != std::this_thread::get_id(), "async properly run");
	}, "EventThreadedProvider - asyncOnce and defer");
	
	return 0;
}

