// A generic ThreadPool implementation

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <queue>
#include <vector>
#include <pthread.h>
#include <cstdlib>
#include <cassert>

// Wrapper around std::queue with some mutex protection
template <typename Data_t>
class WorkQueue {
public:
  WorkQueue()
  {
    pthread_mutex_init(&qmtx,0);

    // wcond is a condition variable that's signaled
    // when new work arrives
    pthread_cond_init(&wcond, 0);
  }

  ~WorkQueue() {
    pthread_mutex_destroy(&qmtx);
    pthread_cond_destroy(&wcond);
  }

  Data_t* next() {
    // Lock the queue mutex
    pthread_mutex_lock(&qmtx);

    while (buffers.empty())
      pthread_cond_wait(&wcond, &qmtx);

    Data_t* nt = buffers.front();
    buffers.pop();
    // Unlock the mutex and return
    pthread_mutex_unlock(&qmtx);
    return nt;
  }
  void add( Data_t* nt ) {
    // Lock the queue
    pthread_mutex_lock(&qmtx);
    // Push task data onto the queue
    buffers.push(nt);
    // signal there's new work
    pthread_cond_signal(&wcond);
    // Unlock the mutex
    pthread_mutex_unlock(&qmtx);
  }

private:
  std::queue<Data_t*> buffers;
  pthread_mutex_t qmtx;
  pthread_cond_t wcond;
};

template <typename Data_t>
class Thread {
public:
  Thread(WorkQueue<Data_t>& _work_queue, WorkQueue<Data_t>& _free_queue) :
    work_queue(_work_queue), free_queue(_free_queue), state(kNone), handle(0)
  {}
  virtual ~Thread()
  { assert(state == kJoined); }

  void start()
  {
    assert(state == kNone);
    if (pthread_create(&handle, NULL, threadProc, this))
      abort();
    state = kStarted;
  }

  void join()
  {
    // A started thread must be joined exactly once!
    assert(state == kStarted);
    pthread_join(handle, NULL);
    state = kJoined;
  }

protected:
  virtual void run() = 0;

  WorkQueue<Data_t>& work_queue;
  WorkQueue<Data_t>& free_queue;

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
      threads.push_back(new Thread_t<Data_t>(workQueue,freeQueue));
      threads.back()->start();
    }
  }

  // Wait for the threads to finish, then delete them
  ~ThreadPool() {
    for (size_t i=0,e=threads.size(); i<e; ++i)
      workQueue.add(NULL);
    for (size_t i=0,e=threads.size(); i<e; ++i) {
      threads[i]->join();
      delete threads[i];
    }
  }

  // Add data to process
  void Process( Data_t* nt ) {
    workQueue.add(nt);
  }

  void addFreeData( Data_t* nt ) {
    freeQueue.add(nt);
  }

  Data_t* nextFree() {
    return freeQueue.next();
  }

private:
  std::vector<Thread<Data_t>*> threads;
  WorkQueue<Data_t> workQueue;
  WorkQueue<Data_t> freeQueue;
};

#endif
