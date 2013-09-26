cpp-EventEmitter
============

Attach lightweight C++11 event emitting and handling capabilities to your existing C++ classes.

* Compatible with GCC 4.7 and newer, Visual Studio 2013 (tested with RC) and Clang++ (with some multi-threading issues)
* Single header file that you can copy to your codebase
* Attach event handlers with lambda functions and construct very readable, elegant and concise code.
* Define new emitters with a `DefineEventEmitter` macro to use methods such as `emitChatMessage`, `onChatMessage` or use `EventEmitter<Args>` template to define an event emitting member with methods `on`, `trigger`.
* Leak-safe, uses shared pointers all over the place.
* Different classes for different uses.

EventEmitter class
============
* Events are immediately called upon `trigger`.
* `sizeof(void*)` overhead for non-initialized emitter and `3 * sizeof(void*)` per each attached handler.
* Lightweight.

DeferredEventEmitter class
============
* Events are cached upon `trigger` and run when called `runDeferred()` or `runAllDeferred()`. Useful when a different thread is a producer of events but you want the handlers to run in another thread.
* Thread safe, mutex protected methods.

ThreadedEventEmitter class
============
* Base EventEmitter functionality and DeferredEventEmitter compiled, the latter under `defer` instead of `trigger`.
* Utilities for waiting for events, getting future results as `std::future`, adding async handlers and general thread safety.

EventDispatcher
============
* Similiar to EventEmitter but dispatch events based on first argument, for example `std::string`.
