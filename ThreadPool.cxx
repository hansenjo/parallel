// A generic ThreadPool implementation

#include "ThreadPool.h"

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <unistd.h>
#include <malloc.h>

using namespace std;

// stdout is a shared resource, so protect it with a mutex
//static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;

// Reusable thread class
class Thread
{
public:
  Thread()
  {
    state = EState_None;
    handle = 0;
  }

  ~Thread()
  {
    assert(state != EState_Started || joined);
  }

  void start()
  {
    assert(state == EState_None);
    // in case of thread create error I usually FatalExit...
    if (pthread_create(&handle, NULL, threadProc, this))
      abort();
    state = EState_Started;
  }

  void join()
  {
    // A started thread must be joined exactly once!
    assert(state == EState_Started);
    pthread_join(handle, NULL);
    state = EState_Joined;
  }

protected:
  virtual void run() = 0;

private:
  static void* threadProc(void* param)
  {
    Thread* thread = reinterpret_cast<Thread*>(param);
    thread->run();
    return NULL;
  }

private:
  enum EState
    {
      EState_None,
      EState_Started,
      EState_Joined
    };

  EState state;
  bool joined;
  pthread_t handle;
};


WorkQueue::WorkQueue()
{
  pthread_mutex_init(&qmtx,0);

  // wcond is a condition variable that's signaled
  // when new work arrives
  pthread_cond_init(&wcond, 0);
}

WorkQueue::~WorkQueue()
{
  // Cleanup pthreads
  pthread_mutex_destroy(&qmtx);
  pthread_cond_destroy(&wcond);
}

// Retrieves the next task from the queue
Task* WorkQueue::nextTask()
{
  // The return value
  Task* nt = 0;

  // Lock the queue mutex
  pthread_mutex_lock(&qmtx);

  while (tasks.empty())
    pthread_cond_wait(&wcond, &qmtx);

  nt = tasks.front();
  tasks.pop();

  // Unlock the mutex and return
  pthread_mutex_unlock(&qmtx);
  return nt;
}
// Add a task
void WorkQueue::addTask( Task* nt ) {
  // Lock the queue
  pthread_mutex_lock(&qmtx);
  // Add the task
  tasks.push(nt);
  // signal there's new work
  pthread_cond_signal(&wcond);
  // Unlock the mutex
  pthread_mutex_unlock(&qmtx);
}

// Thanks to the reusable thread class implementing threads is
// simple and free of pthread api usage.
class PoolWorkerThread : public Thread
{
public:
  PoolWorkerThread(WorkQueue& _work_queue) : work_queue(_work_queue) {}
protected:
  virtual void run()
  {
    while (Task* task = work_queue.nextTask())
      task->run();
  }
private:
  WorkQueue& work_queue;
};

// Allocate a thread pool and set them to work trying to get tasks
ThreadPool::ThreadPool( int n )
{
  finishing = false;
  printf("Creating a thread pool with %d threads\n", n);
  for (int i=0; i<n; ++i) {
    threads.push_back(new PoolWorkerThread(workQueue));
    threads.back()->start();
  }
}

// Wait for the threads to finish, then delete them
ThreadPool::~ThreadPool() {
  finish();
}

// Add a task
void ThreadPool::addTask( Task* nt ) {
  workQueue.addTask(nt);
}

// Tell the tasks to finish and return
void ThreadPool::finish() {
  for (size_t i=0,e=threads.size(); i<e; ++i)
    workQueue.addTask(NULL);
  for (size_t i=0,e=threads.size(); i<e; ++i) {
    threads[i]->join();
    delete threads[i];
  }
  threads.clear();
}

