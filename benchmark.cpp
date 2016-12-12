#include "EventEmitter.hpp"

#include <cassert>

DefineDeferredEventEmitter(Test)

int main(void)
{
	TestDeferredEventEmitter provider;
	
	int counter[10] = {0,};
	auto setupHandlers = [&] {
		for(int i = 0;i < 10;++i) {
			provider.onceTest([&, i]() {
				counter[i]++;
			});
		}
	};
	setupHandlers();
	for(int i =0; i < 100000;++i) {
		provider.triggerTest();
		provider.runAllDeferred();
		setupHandlers();
	}
	provider.runAllDeferred();
	for(int i = 0;i < 10;++i) {
		assert(counter[i] == 100000);
	}
	return 0;
}
