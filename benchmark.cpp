#include "EventEmitter.hpp"

#include <cassert>

DefineDeferredEventEmitter(Test)

int main(void)
{
	TestDeferredEventEmitter provider;

	static int count = 1000000;

	int counter[10] = {0,};
	auto setupHandlers = [&] {
		for(int i = 0;i < 10;++i) {
			provider.onceTest([&, i]() {
				counter[i]++;
			});
		}
	};
	setupHandlers();
	for(int i =0; i < count;++i) {
		provider.triggerTest();
		provider.runAllDeferred();
		setupHandlers();
	}
	provider.runAllDeferred();
	for(int i = 0;i < 10;++i) {
		assert(counter[i] == count);
	}
	return 0;
}
