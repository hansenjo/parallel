// A generic QueuingThreadPool implementation
//
// This class implements a thread pool operating on two queues. Each thread
// runs an identical thread worker function. The worker function is part of
// a configurable "action" object. Each thread holds a separate copy of this
// object. Data to be processed by the worker function are put in the pool's
// "work queue" (input queue), from where they are picked up by the next
// available worker thread. After processing, the data are put in the "result
// queue" (output queue). The calling program is responsible for adding
// data objects to the work queue and removing them from the result queue.
// The worker function must terminate when receiving a nullptr from the
// work queue.
//
// Requires C++17.
//
// Ole Hansen <ole@jlab.org> 12-Feb-2015
//
// To use this class:
// (1) Define the type of data each worker thread should process. This can
//     be a basic type, a structure, a class etc. Example:
//
//     struct Data_t {
//        int input;
//        float result;
//     };
//
// (2) Write a worker thread implementation. This is usually a class (but could
//     be a free function) that implements a run() method.  The run() method
//     should take Data_t items from the the pool's work queue, process
//     them, and put the processed items into pool's result queue.
//
//     #include "QueuingThreadPool.hpp"
//     #include <memory>
//
//     template< typename T>
//     class WorkerThread {
//     public:
//        WorkerThread() = default;
//     ....
//        void run( ThreadUtil::QueuingThreadPool<T>* pool )
//        {
//           while( auto data = pool->pop_work() ) {
//             // Do something with data. "data" is a unique_ptr<Data_t>.
//             data->result = sqrt(data->input);
//
//             // Once processed, move the data into the results queue
//             pool->push_result(std::move(data));
//        }
//     ....
//     // The class may have data members, which must be copyable and movable.
//     // (Implement copy and move constructors for WorkerThread if necessary.
//     // Be aware that each thread will hold a copy of these data, so avoid
//     // large memory allocations.
//     };
//
// (3) Instantiate the thread pool.
//
//     ThreadUtil::QueuingThreadPool<Data_t>
//         pool( NTHREADS, WorkerThread<Data_t>() );
//
// (4) Allocate the data as a unique_ptr<Data_t>. Initialize as necessary.
//     Example:
//
//     std::unique_ptr<Data_t> data(new Data_t);
//
// (5) Add the data to the pool's work queue. (This would usually be done
//     inside a loop.)
//
//     pool.push_work( std::move(data) );
//
// (6) Pick up the processed data from the pool's result queue:
//
//     data = pool.pop_result();
//

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <cassert>
#include <memory>

namespace ThreadUtil {

// Thread-safe queue with fine-grained locking. Holds objects of type
// std::unique_ptr<Data_t>. The Data_t ownership is passed back and forth
// between caller and queue as the unique_ptrs are pushed and popped.
//
// Adapted from "C++ Concurrency in Action", 2nd ed., A. Williams,
// Manning, 2019, ch. 6
template<typename Data_t>
class WorkQueue {
public:
  WorkQueue() : head(new Node), tail(head.get()) {}
  WorkQueue( const WorkQueue& ) = delete;
  WorkQueue& operator=( const WorkQueue& ) = delete;

  // Fetch data if available, otherwise return nullptr
  std::unique_ptr<Data_t> try_pop() {
    std::unique_ptr<Node> const old_head = try_pop_head();
    return old_head ? std::move(old_head->data) : nullptr;
  }
  // Wait until data available, then return it
  std::unique_ptr<Data_t> wait_and_pop() {
    std::unique_ptr<Node> const old_head = wait_pop_head();
    return std::move(old_head->data);
  }
  // Push data onto the queue
  void push( std::unique_ptr<Data_t>&& new_data ) {
    std::unique_ptr<Node> p(new Node);
    {
      std::lock_guard<std::mutex> tail_lock(tail_mutex);
      tail->data = std::move(new_data);
      Node* const new_tail = p.get();
      tail->next = std::move(p);
      tail = new_tail;
    }
    data_cond.notify_one();
  }
  // Convenience functions
  std::unique_ptr<Data_t> next() { return wait_and_pop(); }
  void add( std::unique_ptr<Data_t>&& data ) { push(std::move(data)); }

private:
  struct Node {
    Node() = default;
    std::unique_ptr<Data_t> data;
    std::unique_ptr<Node> next;
  };

  std::mutex head_mutex;
  std::unique_ptr<Node> head;
  std::mutex tail_mutex;
  Node* tail;
  std::condition_variable data_cond;

  Node* get_tail() {
    std::lock_guard<std::mutex> tail_lock(tail_mutex);
    return tail;
  }
  std::unique_ptr<Node> pop_head() {
    std::unique_ptr<Node> old_head = std::move(head);
    head = std::move(old_head->next);
    return old_head;
  }
  std::unique_ptr<Node> wait_pop_head() {
    std::unique_lock<std::mutex> head_lock(head_mutex);
    while( head.get() == get_tail() )
      data_cond.wait(head_lock);
    return pop_head();
  }
  std::unique_ptr<Node> try_pop_head() {
    std::lock_guard<std::mutex> head_lock(head_mutex);
    if( head.get() == get_tail() ) {
      return nullptr;
    }
    return pop_head();
  }
};

template<typename Data_t>
class QueuingThreadPool {
public:
  // Normal constructor, using internal ResultQueue
  template<template<typename> class Action, typename T, typename... Args>
  QueuingThreadPool( size_t n, const Action<T>& action, Args&& ... args )
          : fResultQueue(std::make_shared<WorkQueue<Data_t>>()) {
    AddThreads(n, action, std::forward<Args>(args)...);
  }
  // Constructor with shared_ptr to external ResultQueue
  template<template<typename> class Action, typename T, typename... Args>
  QueuingThreadPool( size_t n, std::shared_ptr<WorkQueue<Data_t>> rq,
                     const Action<T>& action, Args&& ... args )
          : fResultQueue(rq) {
    AddThreads(n, action, std::forward<Args>(args)...);
  }
  // Constructor with reference to external ResultQueue
  template<template<typename> class Action, typename T, typename... Args>
  QueuingThreadPool( size_t n, WorkQueue<Data_t>& rq,
                     const Action<T>& action, Args&& ... args )
          : fResultQueue(&rq) {
    AddThreads(n, action, std::forward<Args>(args)...);
  }

  ~QueuingThreadPool() {
    finish();
  }

  // Queue up data for processing
  void push_work( std::unique_ptr<Data_t> data ) {
    fWorkQueue.push(std::move(data));
  }
  // Fetch data to be processed
  std::unique_ptr<Data_t> pop_work() {
    return fWorkQueue.wait_and_pop();
  }
  // Queue up processed data
  void push_result( std::unique_ptr<Data_t> data ) {
    fResultQueue->push(std::move(data));
  }
  // Retrieve processed data
  std::unique_ptr<Data_t> pop_result() {
    return fResultQueue->wait_and_pop();
  }

  WorkQueue<Data_t>& GetWorkQueue() { return fWorkQueue; }
  WorkQueue<Data_t>& GetResultQueue() { return *fResultQueue; }

  // Tell the threads to finish, then delete them
  void finish() {
    for( size_t i = 0, e = fThreads.size(); i < e; ++i )
      // The thread worker functions must terminate when
      // they pick up a nullptr from the work queue.
      push_work(nullptr);
    for( auto& t : fThreads )
      t.join();
    fThreads.clear();
  }

private:
  std::vector<std::thread> fThreads;
  WorkQueue<Data_t> fWorkQueue;
  std::shared_ptr<WorkQueue<Data_t>> fResultQueue;

  template<template<typename> class Action, typename T, typename... Args>
  void AddThreads( size_t n, const Action<T>& action, Args&& ... args ) {
    fThreads.reserve(n);
    for( size_t i = 0; i < n; ++i ) {
      // Spawn threads that run Action::run() with the given arguments
      fThreads.emplace_back(&Action<T>::run, action, this, args...);
    }
  }
};

#if __cplusplus >= 201701L
// Template argument deduction guide
template<template<typename> class Action, typename T, typename... Args>
QueuingThreadPool( size_t, Action<T>, Args... ) -> QueuingThreadPool<T>;
#endif /* __cplusplus >= 201701L */

} // end namespace QueuingThreadPool

#endif
