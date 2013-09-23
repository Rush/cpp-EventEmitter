#ifndef __EVENTEMITTER_SANE_HPP
#define __EVENTEMITTER_SANE_HPP

#ifdef __EVENTEMITTER_TESTING
#undef __EVENTEMITTER_PROVIDER
#undef __EVENTEMITTER_PROVIDER_THREADED
#undef __EVENTEMITTER_PROVIDER_DEFERRED
#endif

#include <functional>
#include <forward_list>
#include <set>
#include <unordered_set>
#include <memory>
#include <deque>


#include <mutex>
#ifndef EVENTEMITTER_DISABLE_THREADING
#include <condition_variable>
#include <future>
#endif

#define __EVENTEMITTER_CONCAT_IMPL(x, y) x ## y
#define __EVENTEMITTER_CONCAT(x, y) __EVENTEMITTER_CONCAT_IMPL(x, y)

#ifndef __EVENTEMITTER_NONMACRO_DEFS
#define __EVENTEMITTER_NONMACRO_DEFS
namespace EE {
	
	template <typename T, typename... Args>
	std::unique_ptr<T> make_unique_helper(std::false_type, Args&&... args) {
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}

	template <typename T, typename... Args>
	std::unique_ptr<T> make_unique_helper(std::true_type, Args&&... args) {
		static_assert(std::extent<T>::value == 0,
									"make_unique<T[N]>() is forbidden, please use make_unique<T[]>().");
		
		typedef typename std::remove_extent<T>::type U;
		return std::unique_ptr<T>(new U[sizeof...(Args)]{std::forward<Args>(args)...});
	}
	
	template <typename T, typename... Args>
	std::unique_ptr<T> make_unique(Args&&... args) {
		return make_unique_helper<T>(std::is_array<T>(), std::forward<Args>(args)...);
	}
	
	
	class DeferredBase {
		// TODO: disable mutex when EVENTEMITTER_DISABLE_THREADING
	protected: 
		typedef std::function<void ()> DeferredHandler;
		std::deque<DeferredHandler>* deferredQueue = nullptr;
		std::mutex mutex;
		inline void assureContainer() {
			if(!deferredQueue)
				deferredQueue = new std::deque<DeferredHandler>();
		}
	protected:
		virtual ~DeferredBase() {
			std::lock_guard<std::mutex> guard(mutex);
			if(deferredQueue)
				delete deferredQueue;
		}
		void runDeferred(DeferredHandler f) {
			std::lock_guard<std::mutex> guard(mutex);
			assureContainer();
			deferredQueue->push_back(f);
		}
	public:
		void clearDeferred() {
			std::lock_guard<std::mutex> guard(mutex);
			assureContainer();
			deferredQueue->clear();
		}
		bool runDeferred() {
			std::lock_guard<std::mutex> guard(mutex);
			assureContainer();
			if(deferredQueue->size()) {
				((deferredQueue->front()))();
				deferredQueue->pop_front();
			}
			return deferredQueue->size();
		}
		void runAllDeferred() {
			while(runDeferred());
		}
	};
	
	// reference_wrapper needs to be use instead of std::reference_wrapper
	// this is because of VS2013 (RC) bug
	template<class T> class reference_wrapper
	{
	public:
		typedef T type;
		explicit reference_wrapper(T& t): t_(&t) {}
		operator T& () const { return *t_; }
		T& get() const { return *t_; }
		T* get_pointer() const { return t_; }
private:
    T* t_;
};
	

  // source: stackoverflow.com/questions/15501301/binding-function-arguments-in-c11
	template<typename T> struct forward_as_ref_type {
   typedef T &&type;
	};
	template<typename T> struct forward_as_ref_type<T &> {
		typedef reference_wrapper<T> type;
	};
  template<typename T> typename forward_as_ref_type<T>::type forward_as_ref(
   typename std::remove_reference<T>::type &t) {
      return static_cast<typename forward_as_ref_type<T>::type>(t);
   }
   template<typename T> T &&forward_as_ref(
   typename std::remove_reference<T>::type &&t) {
      return t;
	 }
 // 	template<class T>
 //   std::reference_wrapper<T> maybe_ref(T& v, int){ return std::ref(v); }
 //   template<class T> T&& maybe_ref(T&& v, long){ return std::forward<T>(v); }
 // ^^^^^^ alternative way, possibly more hackish
	
	template<typename... Args>
	class LambdaCallbackWrapper
	{
		std::function<void(Args...)> m_f;
		std::function<void()> m_afterCb;
	public:
		LambdaCallbackWrapper(const std::function<void(Args...)>& f, const std::function<void()>& afterCb) : m_f(f), m_afterCb(afterCb) {}
		void operator()(Args... fargs) const { m_f(fargs...); m_afterCb();}
	};
	template<typename... Args>
	LambdaCallbackWrapper<Args...> wrapLambdaWithCallback(const std::function<void(Args...)>& f, const std::function<void()>& afterCb) {
		return LambdaCallbackWrapper<Args...>(f, afterCb);
	};
	template<typename... Args>
	class LambdaBeforeCallbackWrapper
	{
		std::function<void(Args...)> m_f;
		std::function<void()> m_beforeCb;
	public:
		LambdaBeforeCallbackWrapper(const std::function<void(Args...)>& f, const std::function<void()>& beforeCb) : m_f(f), m_beforeCb(beforeCb) {}
		void operator()(Args... fargs) const { m_beforeCb(); m_f(fargs...);}
	};
	template<typename... Args>
	LambdaBeforeCallbackWrapper<Args...> wrapLambdaWithBeforeCallback(const std::function<void()>& beforeCb, const std::function<void(Args...)>& f) {
		return LambdaCallbackWrapper<Args...>(f, beforeCb);
	};

#ifndef EVENTEMITTER_DISABLE_THREADING
	
	// TODO: allow callback for setting if async has completed
	template<typename... Args>
	class LambdaAsyncWrapper
	{
		std::function<void(Args...)> m_f;		
	public:
		LambdaAsyncWrapper(const std::function<void(Args...)>& f) : m_f(f) {}
		void operator()(Args... fargs) const { 
			std::async(std::launch::async, m_f, fargs...);
		}
	};
	template<typename... Args>
	LambdaAsyncWrapper<Args...> wrapLambdaInAsync(const std::function<void(Args...)>& f) {
		return LambdaAsyncWrapper<Args...>(f);
	};
	
	template<typename... Args>
	class LambdaPromiseWrapper
	{
		std::shared_ptr<std::promise<std::tuple<Args...>>> m_promise;
	public:
		LambdaPromiseWrapper(std::shared_ptr<std::promise<std::tuple<Args...>>> promise) : m_promise(promise) {}
		void operator()(Args... fargs) const { 
			m_promise->set_value(std::tuple<Args...>(fargs...));
		}
	};
	template<typename... Args>
	LambdaPromiseWrapper<Args...> getLambdaForFuture(std::shared_ptr<std::promise<std::tuple<Args...>>> promise) {
		return LambdaPromiseWrapper<Args...>(promise);
	};

#endif // EVENTEMITTER_DISABLE_THREADING
	
}
#endif // __EVENTEMITTER_NONMACRO_DEFS


#ifndef __EVENTEMITTER_CONTAINER
#define __EVENTEMITTER_CONTAINER std::forward_list<HandlerPtr>
#endif

#define __EVENTEMITTER_PROVIDER(frontname, name) //^//
template<typename... Rest>
class ExampleEventEmitterTpl {
public:
	typedef std::function<void(Rest...)> Handler;
	typedef std::unique_ptr<Handler> HandlerPtr;
	typedef Handler* Handle;
private:
 	struct EventHandlersSet : public __EVENTEMITTER_CONTAINER {
		EventHandlersSet() : eraseLast(false) {}
 		bool eraseLast : 8;
 	};
	EventHandlersSet* eventHandlers = nullptr;
public:
	Handle onExample (Handler handler) {
	  if(!eventHandlers)
			eventHandlers = new EventHandlersSet;
		eventHandlers->emplace_front(EE::make_unique<Handler>(handler));
		return eventHandlers->front().get();
	}
	Handle onceExample (Handler handler) {
	  if(!eventHandlers)
			eventHandlers = new EventHandlersSet;
		eventHandlers->emplace_front( EE::make_unique<Handler>(EE::wrapLambdaWithCallback(handler, [=]() {
			eventHandlers->eraseLast = true;
		})));
		return eventHandlers->front().get();
	}
	template<typename... Args> inline void emitExample (Args&&... fargs) {
		triggerExample(fargs...);
	}
	template<typename... Args> inline void triggerExample (Args&&... fargs) {
	  if(!eventHandlers)
			return;
		auto prev = eventHandlers->before_begin(); 
	  for(auto i = eventHandlers->begin();i != eventHandlers->end();) {
			
			(*(*i))(fargs...);
			if(eventHandlers->eraseLast) {
				i = eventHandlers->erase_after(prev);
				eventHandlers->eraseLast = false;
			}
			else {
				++i;
				++prev;
			}
		}
	}
	bool removeExampleHandler (Handle handlerPtr) {
 	  if(eventHandlers) { 			
 			auto prev = eventHandlers->before_begin(); 
			for(auto i = eventHandlers->begin();i != eventHandlers->end();++i,++prev) { 
				if((i)->get() == handlerPtr) {
					eventHandlers->erase_after(prev);
					return true;
				}
			}
 		}
 		return false;
	}
	void removeAllExampleHandlers () {
		if(eventHandlers)
			eventHandlers->clear();
	}
	~ExampleEventEmitterTpl() {
		if(eventHandlers) {
			delete eventHandlers;
		}
	}
}; //_//

#define __EVENTEMITTER_PROVIDER_DEFERRED(frontname, name) //^//
template<typename... Rest>
class ExampleDeferredEventEmitterTpl : public ExampleEventEmitterTpl<Rest...>, public virtual EE::DeferredBase {
public:

	template<typename... Args> inline void emitExample (Args&&... fargs) {
		triggerExample(fargs...);
	}
	template<typename... Args> void triggerExample (Args&&... fargs) {
		runDeferred(
			std::bind([=](Args... as) {
			this->ExampleEventEmitterTpl<Rest...>::triggerExample(as...);
			}, EE::forward_as_ref<Args>(fargs)...));
	}
}; //_//

#ifndef EVENTEMITTER_DISABLE_THREADING

#define __EVENTEMITTER_PROVIDER_THREADED(frontname, name) //^//
template<typename... Rest>
class ExampleThreadedEventEmitterTpl : public ExampleEventEmitterTpl<Rest...>, public virtual EE::DeferredBase { 
  std::condition_variable condition;
	std::mutex m;

	typedef typename ExampleEventEmitterTpl<Rest...>::Handler Handler;
	typedef typename ExampleEventEmitterTpl<Rest...>::HandlerPtr HandlerPtr;
	typedef typename ExampleEventEmitterTpl<Rest...>::Handle Handle;
	
public:
	ExampleThreadedEventEmitterTpl() {
	}	
	bool waitExample (std::chrono::milliseconds duration = std::chrono::milliseconds::max()) {
		waitExample([=](Rest...) {
		}, duration);
 	}
 	bool waitExample (Handler handler, std::chrono::milliseconds duration = std::chrono::milliseconds::max()) {
		std::shared_ptr<std::atomic<bool>> finished = std::make_shared<std::atomic<bool>>();
		std::unique_lock<std::mutex> lk(m);
		Handle ptr = onceExample(
			EE::wrapLambdaWithCallback(handler, [=]() {
				finished->store(true);
				this->condition.notify_all();
		}));
		
		if(duration == std::chrono::milliseconds::max()) {
			condition.wait(lk);
		} 
		else {
			condition.wait_for(lk, duration, [=]() {
				return finished->load();
			});
		}
		bool gotFinished = finished->load();
		if(!gotFinished) {
			ExampleEventEmitterTpl<Rest...>::removeExampleHandler(ptr);
		}
		return gotFinished;
 	}
	void asyncWaitExample(Handler handler, std::chrono::milliseconds duration, const std::function<void()>& asyncTimeout) {
		auto async = std::async(std::launch::async, [=]() {
			if(!waitExample(handler, duration))
				asyncTimeout();
		});
	}
	
	Handle onExample (Handler handler) {
		std::lock_guard<std::mutex> guard(mutex);
		return ExampleEventEmitterTpl<Rest...>::onExample(handler);
	}
	Handle onceExample (Handler handler) {
		std::lock_guard<std::mutex> guard(mutex);
		return ExampleEventEmitterTpl<Rest...>::onceExample(handler);
	}
	Handle asyncOnExample (Handler handler) {
		return onExample(EE::wrapLambdaInAsync(handler));
	}
	Handle asyncOnceExample (Handler handler) {
		return onceExample(EE::wrapLambdaInAsync(handler));
	}
	auto futureOnceExample() -> decltype(std::future<std::tuple<Rest...>>()) {
		typedef std::tuple<Rest...> TupleEventType;
		auto promise = std::make_shared<std::promise<TupleEventType>>();
		auto future = promise->get_future();
		condition.notify_all();
		onceExample(EE::getLambdaForFuture(promise));
		return future;
	}
	template<typename... Args> inline void emitExample (Args&&... fargs) {
		triggerExample(fargs...);
	}
	template<typename... Args> void triggerExample (Args&&... fargs) { 
		std::lock_guard<std::mutex> guard(mutex);
		ExampleEventEmitterTpl<Rest...>::triggerExample(fargs...);
		condition.notify_all();
	}
	template<typename... Args> void deferExample (Args&&... fargs) { 
		runDeferred(
 			std::bind([=](Args... as) {
 			// NOTE: with this it does not fail!!! without this it fails
 			this->ExampleEventEmitterTpl<Rest...>::triggerExample(as...);
 			},
			EE::forward_as_ref<Args>(fargs)...			
			//fargs...
			));
	}
}; //_//

#endif // EVENTEMITTER_DISABLE_THREADING

#include <map>

#define EventDispatcherBase ExampleEventEmitter //#//

 #define __EVENTEMITTER_DISPATCHER(frontname, name) //^//
template<template<typename...> class EventDispatcherBase, typename T, typename... Rest>
class ExampleEventDispatcherTpl : public EventDispatcherBase<T, Rest...> {
 	typedef typename EventDispatcherBase<Rest...>::HandlerPtr HandlerPtr;
	typedef typename EventDispatcherBase<Rest...>::Handler Handler;
	typedef typename EventDispatcherBase<Rest...>::Handle Handle;
	std::multimap<T, HandlerPtr> map;
	bool eraseLast = false;
public:
	ExampleEventDispatcherTpl() {
		ExampleEventEmitter<T, Rest...>::onExample([&](T eventName, Rest... fargs) {
 			auto ret = map.equal_range(eventName);
 			for(auto it = ret.first;it != ret.second;) {
 				(*(it->second))(fargs...);
				if(eraseLast) {
					it = map.erase(it);
					eraseLast = false;
				}
				else {
					++it;
				}
 			}
		});
	}
 	Handle onExample (T eventName, Handler handler) {
		return map.insert(std::pair<T, HandlerPtr>(eventName,  EE::make_unique<Handler>(handler)))->second.get();
 	}
 	Handle onceExample (T eventName, Handler handler) {
		return map.insert(std::pair<T, HandlerPtr>(eventName,  EE::make_unique<Handler>(
			EE::wrapLambdaWithCallback(handler, [&] {
				eraseLast = true;
			}))))->second.get();
 	}
 	bool removeExampleHandler (T eventName, Handle handler) {
		auto ret = map.equal_range(eventName);
		for(auto it = ret.first;it != ret.second;) {
			if(it->second.get() == handler) {
				it = map.erase(it);
				return true;
			}
		}
		return false;
	}
	void removeAllExampleHandlers (T eventName) {
		auto ret = map.equal_range(eventName);
		for(auto it = ret.first;it != ret.second;) {		
			it = map.erase(it);
		}
	}
 }; //_//


#if 0 //#//

#define DefineEventEmitter(name, ...) \
__EVENTEMITTER_PROVIDER(name, name) \
typedef __EVENTEMITTER_CONCAT(name, EventEmitterTpl)<__VA_ARGS__> __EVENTEMITTER_CONCAT(name, EventEmitter);

#define DefineDeferredEventEmitter(name, ...) \
__EVENTEMITTER_PROVIDER(name,name) \
__EVENTEMITTER_PROVIDER_DEFERRED(name,name) \
typedef __EVENTEMITTER_CONCAT(name, DeferredEventEmitterTpl)<__VA_ARGS__> __EVENTEMITTER_CONCAT(name, DeferredEventEmitter);

#define DefineThreadedEventEmitter(name, ...) \
__EVENTEMITTER_PROVIDER(name,name) \
__EVENTEMITTER_PROVIDER_THREADED(name,name) \
typedef __EVENTEMITTER_CONCAT(name, ThreadedEventEmitterTpl)<__VA_ARGS__> __EVENTEMITTER_CONCAT(name, ThreadedEventEmitter);


__EVENTEMITTER_PROVIDER(/**/,/**/)
template<typename... Rest> class EventEmitter : public EventEmitterTpl<Rest...> {};

__EVENTEMITTER_PROVIDER_DEFERRED(/**/,/**/)

template<typename... Rest> class DeferredEventEmitter : public DeferredEventEmitterTpl<Rest...> {};

#ifndef EVENTEMITTER_DISABLE_THREADING
__EVENTEMITTER_PROVIDER_THREADED(/**/,/**/)
#endif

#endif //#//


#endif // __EVENTEMITTER_SANE_HPP