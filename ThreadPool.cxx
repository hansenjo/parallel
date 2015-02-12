// A generic ThreadPool implementation

#include "ThreadPool.h"

#include <iostream>
#include <cstdlib>
#include <cassert>

#include <unistd.h>

using namespace std;

// stdout is a shared resource, so protect it with a mutex
//static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;

// Reusable thread class
Thread::Thread(WorkQueue& _work_queue) :
  work_queue(_work_queue), state(kNone), handle(0)
{
}

Thread::~Thread()
{
  assert(state == kJoined);
}

void Thread::start()
{
  assert(state == kNone);
  if (pthread_create(&handle, NULL, threadProc, this))
    abort();
  state = kStarted;
}

void Thread::join()
{
  // A started thread must be joined exactly once!
  assert(state == kStarted);
  pthread_join(handle, NULL);
  state = kJoined;
}

void* Thread::threadProc(void* param)
{
  Thread* thread = reinterpret_cast<Thread*>(param);
  thread->run();
  return NULL;
}


WorkQueue::WorkQueue( size_t capacity ) : m_capacity(capacity)
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
void* WorkQueue::nextTaskData()
{
  // Lock the queue mutex
  pthread_mutex_lock(&qmtx);

  while (buffers.empty())
    pthread_cond_wait(&wcond, &qmtx);

  void* nt = buffers.front();
  buffers.pop();
//TODO: signal that new work can be added
  // Unlock the mutex and return
  pthread_mutex_unlock(&qmtx);
  return nt;
}

// Add data
//TODO: 
void WorkQueue::addTaskData( void* nt ) {
  // Lock the queue
  pthread_mutex_lock(&qmtx);
//TODO: if queue is "full", wait for elements to be popped
  // Push task data onto the queue
  buffers.push(nt);
  // signal there's new work
  pthread_cond_signal(&wcond);
  // Unlock the mutex
  pthread_mutex_unlock(&qmtx);
}

