#ifndef __EVENTEMITTER_HPP
#define __EVENTEMITTER_HPP

#include <functional>
#include <set>
#include <unordered_set>
#include <memory>
#include <deque>


#ifndef EVENTEMITTER_DISABLE_THREADING
#include <mutex>
#include <condition_variable>
#include <future>
#endif

#define __EVENTEMITTER_CONCAT_IMPL(x, y) x ## y
#define __EVENTEMITTER_CONCAT(x, y) __EVENTEMITTER_CONCAT_IMPL(x, y)




namespace EventEmitter {
	
	class DeferredBase {
	protected: 
		typedef std::function<void ()> DeferredHandler;
		typedef std::unique_ptr<DeferredHandler> DeferredHandlerPtr;
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
	
}


#define __EVENTEMITTER_CONTAINER std::unordered_set<HandlerPtr, std::hash<HandlerPtr>, std::less<HandlerPtr>, std::allocator<HandlerPtr>>

#define __EVENTEMITTER_PROVIDER(name, args)  \
class __EVENTEMITTER_CONCAT(name,EventProvider) { \
public: \
	typedef std::function<void args> Handler; \
	typedef std::shared_ptr<Handler> HandlerPtr; \
private: \
 	struct EventHandlersSet : public __EVENTEMITTER_CONTAINER { \
 		bool eraseLast = false; \
 	}; \
	EventHandlersSet* eventHandlers = nullptr; \
public: \
	HandlerPtr __EVENTEMITTER_CONCAT(on,name) (Handler handler) { \
	  if(!eventHandlers) \
			eventHandlers = new EventHandlersSet; \
		auto i = eventHandlers->insert(std::make_shared<Handler>(handler)); \
		return *(i.first); \
	} \
	HandlerPtr __EVENTEMITTER_CONCAT(once,name) (Handler handler) { \
	  if(!eventHandlers) \
			eventHandlers = new EventHandlersSet; \
		auto i = eventHandlers->insert(std::make_shared<Handler>(EventEmitter::wrapLambdaWithCallback(std::function<void args>(handler), [=]() { \
			eventHandlers->eraseLast = true; \
		}))); \
		return *(i.first); \
	} \
	template<typename... Args> inline void __EVENTEMITTER_CONCAT(trigger,name) (Args&&... fargs) { \
	  if(!eventHandlers) \
			return; \
	  for(auto i = eventHandlers->begin();i != eventHandlers->end();) { \
			(*(*i))(fargs...); \
			if(eventHandlers->eraseLast) { \
				i = eventHandlers->erase(i); \
				eventHandlers->eraseLast = false; \
			} \
			else \
				++i; \
		} \
	} \
	HandlerPtr __EVENTEMITTER_CONCAT(remove,__EVENTEMITTER_CONCAT(name, Handler)) (HandlerPtr handlerPtr) { \
	  if(eventHandlers) {  \
			for(auto i = eventHandlers->begin();i != eventHandlers->end();++i) {  \
				if((i)->get() == handlerPtr.get()) { \
					eventHandlers->erase(i); \
					return handlerPtr; \
				} \
			} \
		} \
	} \
	void __EVENTEMITTER_CONCAT(removeAll,__EVENTEMITTER_CONCAT(name, Handlers)) (bool freeAllMemory = false) { \
	  if(eventHandlers) \
			eventHandlers->clear(); \
		if(freeAllMemory) { \
			delete eventHandlers; \
			eventHandlers = nullptr; \
		} \
	} \
	~__EVENTEMITTER_CONCAT(name,EventProvider)() { \
		if(eventHandlers) { \
			delete eventHandlers; \
		} \
	} \
};  

#define __EVENTEMITTER_PROVIDER_DEFERRED(name, args)  \
class __EVENTEMITTER_CONCAT(name,DeferredEventProvider) : public __EVENTEMITTER_CONCAT(name,EventProvider), public virtual EventEmitter::DeferredBase { \
public: \
	template<typename... Args> void __EVENTEMITTER_CONCAT(trigger,name) (Args&&... fargs) { \
		runDeferred( \
			std::bind([=](Args... as) { \
			this->__EVENTEMITTER_CONCAT(name,EventProvider)::__EVENTEMITTER_CONCAT(trigger,name)(as...); \
			}, EventEmitter::forward_as_ref<Args>(fargs)...)); \
	} \
};  

#define __EVENTEMITTER_PROVIDER_THREADED(name, args)  \
class __EVENTEMITTER_CONCAT(name,ThreadedEventProvider) : public __EVENTEMITTER_CONCAT(name,EventProvider), public virtual EventEmitter::DeferredBase {  \
  std::condition_variable condition; \
	std::mutex m; \
 \
	template <typename R, typename... T> \
	std::tuple<T...> tuple_for_function_args(R (*)(T...)) \
	{ \
		return std::tuple<T...>(); \
	} \
	template <typename R, typename... T> \
	std::future<std::tuple<T...>> future_for_function_args(R (*)(T...)) \
	{ \
		return std::future<std::tuple<T...>>(); \
	} \
 \
	static inline void dummy(int, int, std::string) {} \
public: \
	__EVENTEMITTER_CONCAT(name,ThreadedEventProvider)() { \
	}	 \
	bool __EVENTEMITTER_CONCAT(wait,name) (std::chrono::milliseconds duration = std::chrono::milliseconds::max()) { \
		__EVENTEMITTER_CONCAT(wait,name)([=](int, int, std::string str) { \
		}, duration); \
 	} \
 	bool __EVENTEMITTER_CONCAT(wait,name) (Handler handler, std::chrono::milliseconds duration = std::chrono::milliseconds::max()) { \
		std::shared_ptr<std::atomic<bool>> finished = std::make_shared<std::atomic<bool>>(); \
		std::unique_lock<std::mutex> lk(m); \
		HandlerPtr ptr = __EVENTEMITTER_CONCAT(once,name)( \
			EventEmitter::wrapLambdaWithCallback(handler, [=]() { \
				finished->store(true); \
				condition.notify_all(); \
		})); \
		condition.wait_for(lk, duration, [=]() { \
			return finished->load(); \
		}); \
		bool gotFinished = finished->load(); \
		if(!gotFinished) { \
			__EVENTEMITTER_CONCAT(remove,__EVENTEMITTER_CONCAT(name, Handler))(ptr); \
		} \
		return gotFinished; \
 	} \
	void __EVENTEMITTER_CONCAT(asyncWait,name)(Handler handler, std::chrono::milliseconds duration, const std::function<void()>& asyncTimeout) { \
		auto async = std::async(std::launch::async, [=]() { \
			if(!__EVENTEMITTER_CONCAT(wait,name)(handler, duration)) \
				asyncTimeout(); \
		}); \
	} \
	 \
	HandlerPtr __EVENTEMITTER_CONCAT(on,name) (Handler handler) { \
		std::lock_guard<std::mutex> guard(mutex); \
		return __EVENTEMITTER_CONCAT(name,EventProvider)::__EVENTEMITTER_CONCAT(on,name)(handler); \
	} \
	HandlerPtr __EVENTEMITTER_CONCAT(once,name) (Handler handler) { \
		std::lock_guard<std::mutex> guard(mutex); \
		return __EVENTEMITTER_CONCAT(name,EventProvider)::__EVENTEMITTER_CONCAT(once,name)(handler); \
	} \
	HandlerPtr __EVENTEMITTER_CONCAT(asyncOn,name) (Handler handler) { \
		return __EVENTEMITTER_CONCAT(on,name)(EventEmitter::wrapLambdaInAsync(handler)); \
	} \
	HandlerPtr __EVENTEMITTER_CONCAT(asyncOnce,name) (Handler handler) { \
		return __EVENTEMITTER_CONCAT(once,name)(EventEmitter::wrapLambdaInAsync(handler)); \
	} \
	auto __EVENTEMITTER_CONCAT(futureOnce,name)() -> decltype(future_for_function_args(dummy)) { \
		typedef decltype(tuple_for_function_args(dummy)) TupleEventType; \
		auto promise = std::make_shared<std::promise<TupleEventType>>(); \
		auto future = promise->get_future(); \
		condition.notify_all(); \
		__EVENTEMITTER_CONCAT(once,name)(EventEmitter::getLambdaForFuture(promise)); \
		return future; \
	} \
	template<typename... Args> void __EVENTEMITTER_CONCAT(trigger,name) (Args&&... fargs) {  \
		std::lock_guard<std::mutex> guard(mutex); \
		__EVENTEMITTER_CONCAT(name,EventProvider)::__EVENTEMITTER_CONCAT(trigger,name)(fargs...); \
		std::cout << "Trigerring\n"; \
		condition.notify_all(); \
	} \
	template<typename... Args> void __EVENTEMITTER_CONCAT(defer,name) (Args&&... fargs) {  \
		runDeferred([=](int, int, std::string) { \
			std::cout << "defer Trigerring\n"; \
		}); \
		return; \
		runDeferred( \
 			std::bind([=](Args... as) { \
 			  \
 			this->__EVENTEMITTER_CONCAT(name,EventProvider)::__EVENTEMITTER_CONCAT(trigger,name)(as...); \
 			}, \
			EventEmitter::forward_as_ref<Args>(fargs)...			 \
			  \
			)); \
	} \
};  


#define EventProvider(name, args) \
__EVENTEMITTER_PROVIDER(name, args)

#define EventProviderDeferred(name, args) \
__EVENTEMITTER_PROVIDER(name, args) \
__EVENTEMITTER_PROVIDER_DEFERRED(name, args)

#define EventProviderThreaded(name, args) \
__EVENTEMITTER_PROVIDER(name, args) \
__EVENTEMITTER_PROVIDER_THREADED(name, args)


#endif // __EVENTEMITTER_HPP