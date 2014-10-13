#include "EventEmitter.hpp"

// Suggested non-macro usage
class Widget {
public:
	EventEmitter<int, int> doubleIntEvent;
};


// Suggested macro usage
DefineEventEmitter(DoubleInt, int, int);
class Widget2 : public DoubleIntEventEmitter {
	
};


int main()
{
	Widget w;	
	w.doubleIntEvent.on([=](int a, int b) { // slot
		
	});
	w.doubleIntEvent.trigger(123, 213); // signal
	

	Widget2 w2;
	
	w2.onDoubleInt([=](int a, int b) {
		
	});
	w2.triggerDoubleInt(213, 233);
}