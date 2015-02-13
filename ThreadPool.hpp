// A generic ThreadPool implementation
// Ole Hansen <ole@jlab.org> 12-Feb-2015
//
// To use this class:
// (1) Define the type of data each worker thread should process. This can
//     be a basic type, a structure, a class etc. It should hold the
//     thread-specific data that are controlled by the controller process
//     (manager). Example:
//
//     struct WorkData_t {
//        int input;
//        float result;
//     };
//
// (2) Write a worker thread implementation. This is a class that inherits
//     from ThreadUtil::Thread<Data_t> and implements a run() method. Example:
//
//     #include "ThreadPool.hpp"
//
//     template< typename T>
//     class WorkerThread : public ThreadUtil::Thread<T>
//     {
//     public:
//        WorkerThread( WorkQueue<T>& wq, WorkQueue<T>& fq)
//        : ThreadUtil::Thread<T>(wq,fq) {}
//     ....
//     protected:
//        virtual void run()
//        {
//           while( T* data = this->fWorkQueue.next() ) {
//           // do something with data (which will a pointer to a WorkData_t)
//           // IMPORTANT: once processed, put the data back into the free queue!
//           this->fFreeQueue.add(data)
//        }
//     ....
//     };
//
// (3) Instantiate the thread pool. In the current implementation, the number of
//     threads is fixed. Example:
//
//     ThreadUtil::ThreadPool<WorkerThread,WorkData_t> pool(8);
//
// (4) Allocate at least as many work data objects as there will be worker
//     threads. Initialize as necessary. Example:
//
//     WorkData_t workData[16];
//
// (5) Add these work data objects to the free queue of the pool. Example:
//
//     for( int i=0; i<16; ++i )
//        pool.addFreeData( &workData[i] );
//
// (6) To process data, retrieve a free work data object, put input data into it,
//     and pass it to the pool for processing. Example:
//
//     for( int i=0; i<10000; ++i ) {
//        WorkData_t* data = pool.nextFree();
//        data->input = i;
//        pool.Process(data);
//     }
//
//     Results should be processed within the WorkerThread objects.
//

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <queue>
#include <vector>
#include <pthread.h>
#include <cstdlib>
#include <cassert>

namespace ThreadUtil {

// Thread-safe wrapper around std::queue
template <typename Data_t>
class WorkQueue {
public:
  WorkQueue()
  {
    pthread_mutex_init(&fMutex,0);
    pthread_cond_init(&fWaitCond, 0);
  }

  ~WorkQueue() {
    pthread_mutex_destroy(&fMutex);
    pthread_cond_destroy(&fWaitCond);
  }

  Data_t* next() {
    pthread_mutex_lock(&fMutex);
    // Wait for new work
    while (fQueue.empty())
      pthread_cond_wait(&fWaitCond, &fMutex);

    // Take work data from the front of the queue
    Data_t* data = fQueue.front();
    fQueue.pop();

    pthread_mutex_unlock(&fMutex);
    return data;
  }

  void add( Data_t* data ) {
    pthread_mutex_lock(&fMutex);
    // Push thread work data onto the queue
    fQueue.push(data);
    // signal there's new work
    pthread_cond_signal(&fWaitCond);
    pthread_mutex_unlock(&fMutex);
  }

private:
  std::queue<Data_t*> fQueue;
  pthread_mutex_t fMutex;
  pthread_cond_t  fWaitCond;
};

template <typename Data_t>
class Thread {
public:
  Thread(WorkQueue<Data_t>& work_queue, WorkQueue<Data_t>& free_queue) :
    fWorkQueue(work_queue), fFreeQueue(free_queue), state(kNone), handle(0) {}
  virtual ~Thread() { assert(state == kJoined); }

  void start() {
    assert(state == kNone);
    if (pthread_create(&handle, NULL, threadProc, this))
      abort();
    state = kStarted;
  }

  void join() {
    assert(state == kStarted);
    pthread_join(handle, NULL);
    state = kJoined;
  }

protected:
  virtual void run() = 0;

  WorkQueue<Data_t>& fWorkQueue;
  WorkQueue<Data_t>& fFreeQueue;

private:
  static void* threadProc( void* param )
  {
    Thread* thread = reinterpret_cast<Thread*>(param);
    thread->run();
    return NULL;
  }

  enum EState { kNone, kStarted, kJoined };

  EState state;
  pthread_t handle;

};


template <template<typename> class Thread_t, typename Data_t>
class ThreadPool {
public:
  // Allocate a thread pool and set them to work trying to get tasks
  ThreadPool( size_t n ) {
    for (size_t i=0; i<n; ++i) {
      fThreads.push_back(new Thread_t<Data_t>(fWorkQueue,fFreeQueue));
      fThreads.back()->start();
    }
  }

  // Wait for the threads to finish, then delete them
  ~ThreadPool() {
    for (size_t i=0,e=fThreads.size(); i<e; ++i)
      fWorkQueue.add(NULL);
    for (size_t i=0,e=fThreads.size(); i<e; ++i) {
      fThreads[i]->join();
      delete fThreads[i];
    }
  }

  // Queue up data for processing
  void Process( Data_t* data ) {
    fWorkQueue.add(data);
  }

  void addFreeData( Data_t* data ) {
    fFreeQueue.add(data);
  }

  Data_t* nextFree() {
    return fFreeQueue.next();
  }

private:
  std::vector<Thread<Data_t>*> fThreads;
  WorkQueue<Data_t> fWorkQueue;
  WorkQueue<Data_t> fFreeQueue;
};

} // end namespace ThreadPool

#endif
