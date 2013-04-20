#include "EventEmitter.hpp"

#include <cstdio>

EventProvider(Test, (void))
// class TestEventProvider {
// public: 
// 	typedef std::function<void (void)> Handler; 
// 	typedef std::shared_ptr<Handler> HandlerPtr; 
// 	private:
// 	struct EventHandlersSet : public std::set<HandlerPtr, std::less<HandlerPtr>, std::allocator<HandlerPtr>> {
// 		bool eraseLast = false;
// 	};
// 	EventHandlersSet* eventHandlers = nullptr; 
// public: 
// 	HandlerPtr onTest (Handler handler) {  
// 	  if(!eventHandlers) 
// 			eventHandlers = new EventHandlersSet; 
// 		auto i = eventHandlers->insert(std::make_shared<Handler>(handler)); 
// 		return *(i.first); 
// 	} 
// 	HandlerPtr onceTest(Handler handler) {  
// 	  if(!eventHandlers) 
// 			eventHandlers = new EventHandlersSet; 
// 		auto i = eventHandlers->insert(std::make_shared<Handler>(wrapLambdaWithCallback(std::function<void (void)>(handler), [=]() { 
// 			eventHandlers->eraseLast = true;
// 		}))); 
// 		return *(i.first); 
// 	} 
// 	template<typename... Args> inline void triggerTest (Args&&... fargs) { 
// 	  if(!eventHandlers) 
// 			return; 
// 	  for(auto i = eventHandlers->begin();i != eventHandlers->end();) { 
// 			(*(*i))(); 
// 			if(eventHandlers->eraseLast) {
// 				i = eventHandlers->erase(i);
// 				eventHandlers->eraseLast = false;
// 			}
// 			else
// 				++i;
// 		} 
// 	} 
// 	HandlerPtr removeTestHandler(HandlerPtr handlerPtr) { 
// 	  if(eventHandlers) 
// 			eventHandlers->erase(handlerPtr); 
// 	} 
// 	void removeAllTestHandlers(bool freeAllMemory = false) {  
// 	  if(eventHandlers) 
// 			eventHandlers->clear(); 
// 		if(freeAllMemory) { 
// 			delete eventHandlers; 
// 			eventHandlers = nullptr; 
// 		} 
// 	} 
// 	~TestEventProvider() {
// 		if(eventHandlers) {
// 			delete eventHandlers;
// 		}
// 	}
// };

class Test1 {
	int field1;
public:	
};

class Test2 : public TestEventProvider {
	int field2;
public:
};

int main()
{
	Test2 test;
	
	printf("sizeof %i\n", sizeof(TestEventProvider));
	
	
	test.onTest([=]() {
		printf("Dupa1\n");
	});

	test.onceTest([=]() {
		printf("Dupa2\n");
	});

	test.triggerTest();
	test.triggerTest();
	
		test.removeAllTestHandlers();
	
	
	
	return 0;
}

