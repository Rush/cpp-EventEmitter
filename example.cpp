#include "EventEmitter.hpp"

using namespace EE;

DefineEventEmitter(DoubleInt, int, int);
DefineEventEmitter(Click);

// Suggested usage as a property
class Widget {
public:
	DoubleIntEventEmitter doubleIntEvent;
};


class Widget2 : public DoubleIntEventEmitter, public ClickEventEmitter {
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

	w2.onClick([=] {
		printf("Clicked\n");
	});

	w2.triggerClick();
}