#define _GLIBCXX_USE_NANOSLEEP
#include "EventEmitter.hpp"

#include <exception>
#include <iostream>

class test_exception : public std::exception {
  std::string msg;

public:
  test_exception(const char *_msg) : msg(_msg) {}
  virtual const char *what() const throw() { return msg.c_str(); }
};

template <typename X, typename A>
inline void Assert(A assertion, const char *msg = "") {
  if (!assertion)
    throw X(msg);
}

#undef assert
#define assert Assert<test_exception>

void runTest(std::function<void()> test, const std::string &str) {
  try {
    test();
    std::cout << "[ OK ] " << str.c_str() << "\n";
  } catch (test_exception exception) {
    std::cout << "[Fail] " << str.c_str() << "\n";
    std::cerr << "===========\n" << exception.what() << "\n===========\n";
  }
}

DefineEventEmitter(Example, int, int, std::string)
DefineDeferredEventEmitter(Example2, int, int, std::string)

int main() {
  runTest(
      [] {
        int counter1 = 0, counter2 = 0;
        ExampleEventEmitter test;
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
      },
      "EventEmitter - on, once, trigger");

  runTest(
      [] {
        ExampleEventEmitter test;
        int sum = 0;
        auto lambda = [&](int a, int b, std::string str) {
					sum += a + b;
				};
        auto handler = test.onExample(lambda);
        test.triggerExample(1, 3, "A");
        test.triggerExample(5, 7, "B");
        test.removeExampleHandler(handler);

        assert(sum == 16, "removeExampleHandler: third trigger should not run");
        handler = test.onceExample(lambda);
        test.removeExampleHandler(handler);
        test.triggerExample(11, 13, "B");
        assert(sum == 16,
               "removeExampleHandler: once handler should have been removed");
      },
      "EventEmitter - removeHandler");

  runTest(
      [] {
        ExampleEventEmitter test;
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
      },
      "EventEmitter - removeAllHandlers");

  // TODO: make this work!!
  // 		runTest([] {
  // 			ExampleEventEmitter test;
  //
  // 			int sum = 0;
  // 			ExampleEventEmitter::HandlerPtr ptr =
  // test.onExample([&](int
  // a, int b, std::string str) { 				sum += a;
  // test.removeExampleHandler(ptr);
  // 			});
  // 			test.triggerExample(11, 13, "B");
  // 			assert(sum == 11, "removeExampleHandler: third trigger
  // should not run"); 			assert(sum == 16, "removeExampleHandler:
  // once handler should have been removed");
  // 		}, "EventEmitter - removeHandler from within self");
  runTest(
      [] {
        int counter1 = 0, counter2 = 0;
        Example2DeferredEventEmitter test;
        test.onExample2([&counter1](int a, int b, std::string str) {
					counter1 += a + b;
				});

        test.onceExample2([&counter2](int a, int b, std::string str) {
					counter2 += a + b;
				});

        test.triggerExample2(1, 5, "A");
        test.triggerExample2(3, 7, "B");

				int i = 0;
        assert(counter1 == 0, "on: should not have been called at all");
        assert(counter2 == 0, "on: should not have been called at all");
				// normally we would run "runAllDeferred" - we just want to check the order here
				test.runDeferred(i);

        assert(counter1 == 6, "on: should equal sum of arguments");
        assert(counter2 == 6, "once: should equal sum of arguments");
        test.runDeferred(i);

        assert(counter1 == 16, "on: second trigger should fire");
        assert(counter2 == 6, "once: second trigger should not fire");

        test.removeAllExample2Handlers();
        test.runAllDeferred();
        assert(counter1 == 16, "removeAllHandlers should have removed handler");
      },
      "EventDeferredEmitter - on, once, trigger, removeAllHandlers");

  return 0;
}
