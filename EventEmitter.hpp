#pragma once

#include <forward_list>
#include <functional>
#include <memory>
#include <vector>

namespace EE {
// source:
// stackoverflow.com/questions/15501301/binding-function-arguments-in-c11
template <typename T> struct forward_as_ref_type { typedef T &&type; };
template <typename T> struct forward_as_ref_type<T &> {
  typedef std::reference_wrapper<T> type;
};
template <typename T>
typename forward_as_ref_type<T>::type
forward_as_ref(typename std::remove_reference<T>::type &t) {
  return static_cast<typename forward_as_ref_type<T>::type>(t);
}
template <typename T>
T &&forward_as_ref(typename std::remove_reference<T>::type &&t) {
  return t;
}

namespace detail {
// helper
template <class T> struct derive : T {
  template <class... A> derive(A... a) : T(a...) {}
};

template <class X, class... T> struct deriver;

template <class X> struct deriver<X> : derive<X> {
  template <class... A> deriver(A... a) : derive<X>(a...) {}
};

template <class X, class... T> struct deriver : derive<X>, deriver<T...> {
  template <class... A> deriver(A... a) : derive<X>(a...), deriver<T...>() {}
};
} // namespace detail

struct default_ee_engine {
  template <typename... T> using function = typename std::function<T...>;

  template <typename T> class container {
  public:
    using base = std::forward_list<T>;
    using handle = intptr_t;

    struct node {
      std::unique_ptr<node> next;
      T val;
      node(T &&_val, node *_next) : next(_next), val(std::move(_val)) {}
    };

    std::unique_ptr<node> next;

    struct iterator {
      node *current;
      node *prev;
      bool removeFlag;

      bool operator!=(iterator rhs) { return current != rhs.current; }
      T &operator*() { return current->val; }
      void operator++() {
        auto nextPtr = current->next.get();
        if (removeFlag || (current != (node *)this && !current->val)) {

          current->next.release();
          prev->next.reset(nextPtr);
          current = nextPtr;
        } else {
          prev = current;
          current = nextPtr;
        }
        removeFlag = clearOnceFlag();
      }

      bool hasOnceFlag() {
        return current ? (**(intptr_t **)(&current) & 0x1) : false;
      }
      bool clearOnceFlag() {
        if (!hasOnceFlag()) {
          return false;
        }
        **(intptr_t **)(&current) &= ~0x1;
        return true;
      }

      void setOnceFlag() {
        if (!current) {
          return;
        }
        **(intptr_t **)(&current) |= 0x1;
      }
    };

    iterator begin() {
      iterator i{next.get(), (node *)this};
      i.removeFlag = i.clearOnceFlag();
      return i;
    }
    iterator end() { return {nullptr}; }

  public:
    ~container() {
      for (auto i : *this) {
      }
    }

    bool empty() { return !next; }

    void erase(handle h) {
      auto n = (node *)h;
      n->val = T();
    }

    handle emplace(T &&elem, bool removeFlag = false) {
      auto oldNext = next.release();
      next = std::make_unique<node>(std::move(elem), oldNext);
      auto ptr = (handle)this->next.get();
      if (removeFlag) {
        this->begin().setOnceFlag();
      }
      return ptr;
    }
  };
};

template <typename ee_engine> struct abstract_ee {

  template <typename... T>
  using function = typename ee_engine::template function<T...>;

  template <typename T>
  using container = typename ee_engine::template container<T>;

  class deferred_base {
  protected:
    using deferred_handler = function<void()>;
    std::vector<deferred_handler> deferredQueue;

  protected:
    void runDeferred(deferred_handler &&f) {
      if (deferredQueue.empty()) {
        deferredQueue.reserve(4);
      }
      deferredQueue.emplace_back(std::move(f));
    }

  public:
    void clearDeferred() { deferredQueue.clear(); }
    bool runDeferred(int &idx) {
      if (idx < static_cast<int>(deferredQueue.size())) {
        auto &f = deferredQueue[idx];
        if (f) {
          f();
          f = nullptr;
        }
        idx++;
        return true;
      }
      return false;
    }
    void runAllDeferred() {
      int idx = 0;
      while (runDeferred(idx))
        ;
      deferredQueue.clear();
    }
  };

  template <typename... Args> class event {
  public:
    using event_handler = function<void(Args...)>;

  private:
    using Container = container<event_handler>;

  public:
    using registered_handle = typename Container::handle;
    using Handle = typename Container::handle;

  private:
    Container eventHandlers;

  public:
    registered_handle on(event_handler handler) {
      return eventHandlers.emplace(std::move(handler));
    };

    registered_handle once(event_handler handler) {
      return eventHandlers.emplace(std::move(handler), true);
    }

    void trigger(Args... fargs) {
      for (auto &f : eventHandlers) {
        if (f) {
          f(fargs...);
        }
      }
    };

    bool hasHandlers() { return !eventHandlers.empty(); }

    int countHandlers() {
      int count = 0;
      for (auto &i : eventHandlers)
        count++;
      return count;
    }

    bool removeHandler(registered_handle handlerPtr) {
      eventHandlers.erase(handlerPtr);
      return !eventHandlers.empty();
    }

    int removeAllHandlers() {
      int count = 0;
      // for(;!eventHandlers.empty() &&
      // removeHandler(eventHandlers.before_begin());++count);
      return count;
    }

    ~event() { removeAllHandlers(); }
  };

  template <typename... Args>
  class deferred_event : public event<Args...>, public virtual deferred_base {
  public:
    using event_base = event<Args...>;
    deferred_event() {}
    inline void emit(Args &&... fargs) { trigger(fargs...); }

    template <typename... FArgs> void trigger(FArgs &&... fargs) {
      this->runDeferred([=]() { this->event_base::trigger(fargs...); });
    }
  };

  template <class... Bases>
  class event_emitter : public detail::deriver<Bases...> {
    struct sum_helper {
      template <typename T> static int sum(T n) { return n; }

      template <typename T, typename... Args>
      static int sum(T n, Args... rest) {
        return n + sum(rest...);
      }
    };

    template <class... A> int callRemovalForBases() {
      return [](auto... param) {
        return sum_helper::sum(param...);
      }((A::removeAllHandlers())...);
    }

  public:
    template <class... Args>
    event_emitter(Args... args) : detail::deriver<Bases...>(args...) {}

    template <class T, class... Args>
    typename T::registered_handle on(Args &&... args) {
      return T::on(args...);
    }

    template <class T, class... Args> void trigger(Args &&... args) {
      T::trigger(args...);
    }

    virtual int removeAllHandlers() { return callRemovalForBases<Bases...>(); }
  };
};

using default_ee = abstract_ee<default_ee_engine>;

template <class... Bases>
using EventEmitter = default_ee::event_emitter<Bases...>;

template <class... Bases> using Event = default_ee::event<Bases...>;

template <class... Bases>
using DeferredEvent = default_ee::deferred_event<Bases...>;

using DeferredBase = default_ee::deferred_base;
} // namespace EE

#define __EVENTEMITTER_CONCAT_IMPL(x, y) x##y
#define __EVENTEMITTER_CONCAT(x, y) __EVENTEMITTER_CONCAT_IMPL(x, y)

#define __DefineEventEmitter(class_name, base_class, name, ...)                \
  struct class_name : EE::base_class<__VA_ARGS__> {                            \
    using Base = EE::base_class<__VA_ARGS__>;                                  \
    template <typename... Args>                                                \
    registered_handle __EVENTEMITTER_CONCAT(on, name)(Args && ... args) {      \
      return Base::on(args...);                                                \
    }                                                                          \
    template <typename... Args>                                                \
    registered_handle __EVENTEMITTER_CONCAT(once, name)(Args && ... args) {    \
      return Base::once(args...);                                              \
    }                                                                          \
    template <typename... Args>                                                \
    void __EVENTEMITTER_CONCAT(trigger, name)(Args && ... args) {              \
      Base::trigger(args...);                                                  \
    }                                                                          \
    template <typename... Args>                                                \
    int __EVENTEMITTER_CONCAT(__EVENTEMITTER_CONCAT(removeAll, name),          \
                              Handlers)(Args && ... args) {                    \
      return Base::removeAllHandlers(args...);                                 \
    }                                                                          \
    template <typename... Args>                                                \
    void __EVENTEMITTER_CONCAT(__EVENTEMITTER_CONCAT(remove, name),            \
                               Handler)(Args && ... args) {                    \
      Base::removeHandler(args...);                                            \
    }                                                                          \
  };

#define DefineEventEmitter(name, ...)                                          \
  __DefineEventEmitter(__EVENTEMITTER_CONCAT(name, EventEmitter), Event, name, \
                       __VA_ARGS__)
#define DefineDeferredEventEmitter(name, ...)                                  \
  __DefineEventEmitter(__EVENTEMITTER_CONCAT(name, DeferredEventEmitter),      \
                       DeferredEvent, name, __VA_ARGS__)

#define DefineEventEmitterAs(name, class_name, ...)                            \
  __DefineEventEmitter(class_name, Event, name, __VA_ARGS__)
#define DefineDeferredEventEmitterAs(name, class_name, ...)                    \
  __DefineEventEmitter(class_name, DeferredEvent, name, __VA_ARGS__)
