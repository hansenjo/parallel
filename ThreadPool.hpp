// A generic ThreadPool implementation
// Ole Hansen <ole@jlab.org> 12-Feb-2015
//
// To use this class:
// (1) Define the type of data each worker thread should process. This can
//     be a basic type, a structure, a class etc. It should hold the
//     thread-specific data that are controlled by the controller process
//     (manager). Example:
//
//     struct Data_t {
//        int input;
//        float result;
//     };
//
// (2) Allocate at least as many work data objects as there will be worker
//     threads. Initialize as necessary. Example:
//
//     Data_t workData[16];
//
// (3) Add these work data objects to a "free queue". Example:
//
//     ThreadUtil::WorkQueue<Data_t> freeQueue;
//     for( int i=0; i<16; ++i )
//        freeQueue.add( &workData[i] );
//
// (4) Write a worker thread implementation. This is a class that inherits
//     from ThreadUtil::PoolWorkerThread<Data_t> and implements a run() method.
//     The run()method should take Data_t items from the fWorkQueue, process
//     them, and put the processed Data_t items into fResultQueue.
//     Example:
//
//     #include "ThreadPool.hpp"
//
//     template< typename T>
//     class WorkerThread : public ThreadUtil::PoolWorkerThread<T>
//     {
//     public:
//        WorkerThread( WorkQueue<T>& wq, WorkQueue<T>& fq)
//        : ThreadUtil::PoolWorkerThread<T>(wq,fq) {}
//     ....
//     protected:
//        virtual void run()
//        {
//           while( T* data = this->fWorkQueue.next() ) {
//           // do something with data (which will a pointer to a Data_t)
//           // once processed, put the data into the results queue
//           this->fResultQueue.add(data)
//        }
//     ....
//     };
//
// (5) Instantiate the thread pool. In the current implementation, the number of
//     threads is fixed. Example:
// (5a)
//     ThreadUtil::ThreadPool<WorkerThread,Data_t> pool(8);
//
//     or using the freeQueue as the result queue:
// (5b)
//     ThreadUtil::ThreadPool<WorkerThread,Data_t> pool(8, freeQueue);
//
// (6) To process data, retrieve a free data object, put input data into it,
//     and pass it to the pool for processing. Example, using (5b) above:
//
//     for( int i=0; i<10000; ++i ) {
//        Data_t* d = freeQueue.next();
//        d->input = i;
//        pool.Process(d); // automatically refills freeQueue
//     }
//
//     Results should be processed within the WorkerThread objects.
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

// Thread-safe queue with fine-grained locking. Stores raw pointers to the
// queued objects, which are assumed to be owned by the caller.
// Adapted from "C++ Concurrency in Action", 2nd ed., A. Williams, Manning, 2019, ch. 6
template<typename Data_t>
class WorkQueue {
public:
  WorkQueue() : head(new Node), tail(head.get()) {}
  WorkQueue( const WorkQueue& ) = delete;
  WorkQueue& operator=( const WorkQueue& ) = delete;

  Data_t* try_pop() {
    std::unique_ptr<Node> const old_head = try_pop_head();
    return old_head ? old_head->data : nullptr;
  }
  Data_t* wait_and_pop() {
    std::unique_ptr<Node> const old_head = wait_pop_head();
    return old_head->data;
  }
  void push( Data_t* new_data ) {
    std::unique_ptr<Node> p(new Node);
    {
      std::lock_guard<std::mutex> tail_lock(tail_mutex);
      tail->data = new_data;
      Node* const new_tail = p.get();
      tail->next = std::move(p);
      tail = new_tail;
    }
    data_cond.notify_one();
  }

  // FIXME: old API
  Data_t* next() { return wait_and_pop(); }
  void add( Data_t* data ) { push(data); }

private:
  struct Node {
    Node() : data(nullptr) {}
    Data_t* data;
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
class ThreadPool {
public:
  // Normal constructor, using internal ResultQueue
  template<template<typename> class Action, typename T, typename... Args>
  ThreadPool( size_t n, const Action<T>& action, Args&& ... args )
          : fResultQueue(std::make_shared<WorkQueue<Data_t>>()) {
    AddThreads(n, action, std::forward<Args>(args)...);
  }
  // Constructor with shared_ptr to external ResultQueue
  template<template<typename> class Action, typename T, typename... Args>
  ThreadPool( size_t n, std::shared_ptr<WorkQueue<Data_t>> rq,
              const Action<T>& action, Args&& ... args )
          : fResultQueue(rq) {
    AddThreads(n, action, std::forward<Args>(args)...);
  }
  // Constructor with reference to external ResultQueue
  template<template<typename> class Action, typename T, typename... Args>
  ThreadPool( size_t n, WorkQueue<Data_t>& rq,
              const Action<T>& action, Args&& ... args )
          : fResultQueue(&rq) {
    AddThreads(n, action, std::forward<Args>(args)...);
  }

  ~ThreadPool() {
    finish();
  }

  // Queue up data for processing
  void Process( Data_t* data ) {
    fWorkQueue.add(data);
  }

  WorkQueue<Data_t>& GetWorkQueue() { return fWorkQueue; }
  WorkQueue<Data_t>& GetResultQueue() { return *fResultQueue; }

  // Tell the threads to finish, then delete them
  void finish() {
    for( size_t i = 0, e = fThreads.size(); i < e; ++i )
      // The thread worker functions must be written to terminate as soon as
      // they pick up a nullptr from fWorkQueue.
      fWorkQueue.add(nullptr);
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
ThreadPool( int, Action<T>, Args... ) -> ThreadPool<T>;
#endif /* __cplusplus >= 201701L */

} // end namespace ThreadPool

#endif
