// A generic ThreadPool implementation

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <queue>
#include <vector>
#include <pthread.h>

// Wrapper around std::queue with some mutex protection
//TODO: limit queue size
class WorkQueue {
public:
  WorkQueue( size_t capacity );
  ~WorkQueue();

  void* nextTaskData();
  void  addTaskData( void* nt );

private:
  std::queue<void*> buffers;
  pthread_mutex_t qmtx;
  pthread_cond_t wcond;
  size_t m_capacity;
};

class Thread {
public:
  Thread(WorkQueue& _work_queue);
  virtual ~Thread();

  void start();
  void join();

protected:
  virtual void run() = 0;

  WorkQueue& work_queue;

private:
  static void* threadProc( void* param );

  enum EState { kNone, kStarted, kJoined };

  EState state;
  pthread_t handle;

};


template <typename Thread_t>
class ThreadPool {
public:
  // Allocate a thread pool and set them to work trying to get tasks
  ThreadPool( size_t n ) : workQueue(n) {
    for (size_t i=0; i<n; ++i) {
      threads.push_back(new Thread_t(workQueue));
      threads.back()->start();
    }
  }

  // Wait for the threads to finish, then delete them
  ~ThreadPool() {
    for (size_t i=0,e=threads.size(); i<e; ++i)
      workQueue.addTaskData(NULL);
    for (size_t i=0,e=threads.size(); i<e; ++i) {
      threads[i]->join();
      delete threads[i];
    }
    threads.clear();
  }

  // Add data to process
  void addTaskData( void* nt ) {
    workQueue.addTaskData(nt);
  }

private:
  std::vector<Thread*> threads;
  WorkQueue workQueue;
};

#endif
