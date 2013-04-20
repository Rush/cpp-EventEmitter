#ifndef __EVENTEMITTER_HPP

#include <functional>
#include <set>
#include <memory>

template<typename... Args>
class __LambdaCallbackWrapper
{
  std::function<void(Args...)> m_f;
  std::function<void()> m_afterCb;
public:
  __LambdaCallbackWrapper(std::function<void(Args...)> f, std::function<void()> afterCb) : m_f(f), m_afterCb(afterCb) {}
  void operator()(Args... fargs) const { m_f(fargs...); m_afterCb();}
};
template<typename... Args>
__LambdaCallbackWrapper<Args...> wrapLambdaWithCallback(std::function<void(Args...)> f, std::function<void()> afterCb) {
	return __LambdaCallbackWrapper<Args...>(f, afterCb);
};

#define __EVENTEMITTER_CONCAT_IMPL(x, y) x ## y
#define __EVENTEMITTER_CONCAT(x, y) __EVENTEMITTER_CONCAT_IMPL(x, y)

#define EventProvider(name, args) \
class __EVENTEMITTER_CONCAT(name, EventProvider) { \
public: \
	typedef std::function<void args> Handler; \
	typedef std::shared_ptr<Handler> HandlerPtr; \
	private:\
 	struct EventHandlersSet : public std::set<HandlerPtr, std::less<HandlerPtr>, std::allocator<HandlerPtr>> { \
 		bool eraseLast = false; \
 	}; \
	EventHandlersSet* eventHandlers = nullptr; \
public: \
	HandlerPtr __EVENTEMITTER_CONCAT(on, name) (Handler handler) {  \
	  if(!eventHandlers) \
			eventHandlers = new EventHandlersSet; \
		auto i = eventHandlers->insert(std::make_shared<Handler>(handler)); \
		return *(i.first); \
	} \
	HandlerPtr __EVENTEMITTER_CONCAT(once, name) (Handler handler) {  \
	  if(!eventHandlers) \
			eventHandlers = new EventHandlersSet; \
		auto i = eventHandlers->insert(std::make_shared<Handler>(wrapLambdaWithCallback(std::function<void (void)>(handler), [=]() { \
			eventHandlers->eraseLast = true; \
		}))); \
		return *(i.first); \
	} \
	template<typename... Args> inline void __EVENTEMITTER_CONCAT(trigger, name) (Args&&... fargs) { \
	  if(!eventHandlers) \
			return; \
	  for(auto i = eventHandlers->begin();i != eventHandlers->end();) { \
			(*(*i))(); \
			if(eventHandlers->eraseLast) { \
				i = eventHandlers->erase(i); \
				eventHandlers->eraseLast = false; \
			} \
			else \
				++i; \
		} \
	} \
	HandlerPtr __EVENTEMITTER_CONCAT(__EVENTEMITTER_CONCAT(remove, name), Handler) (HandlerPtr handlerRef) {  \
	  if(eventHandlers) \
			eventHandlers->erase(handlerRef); \
	} \
	void __EVENTEMITTER_CONCAT(__EVENTEMITTER_CONCAT(removeAll, name), Handlers) (bool freeAllMemory = false) {  \
	  if(eventHandlers) \
			eventHandlers->clear(); \
		if(freeAllMemory) { \
			delete eventHandlers; \
			eventHandlers = nullptr; \
		} \
	} \
	~__EVENTEMITTER_CONCAT(name, EventProvider)() { \
		if(eventHandlers) { \
			delete eventHandlers; \
		} \
	} \
};









#endif // __EVENTEMITTER_HPP