#include "EventEmitter.hpp"

#include <cassert>

DefineEventProvider(Test)

int main(void)
{
	TestEventProvider provider;
	
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
		setupHandlers();
	}
	for(int i = 0;i < 10;++i) {
		assert(counter[i] == 100000);
	}
	return 0;
}