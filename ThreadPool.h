// A generic ThreadPool implementation

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <queue>
#include <pthread.h>

// Base task for Tasks
class Task {
public:
    Task() {}
    virtual ~Task() {}

    virtual void run() = 0;
    virtual void showTask() = 0;
};

// Wrapper around std::queue with some mutex protection
class WorkQueue {
public:
  WorkQueue();
  ~WorkQueue();

  Task* nextTask();
  void addTask( Task* nt );

private:
    std::queue<Task*> tasks;
    pthread_mutex_t qmtx;
    pthread_cond_t wcond;
};

class PoolWorkerThread;

class ThreadPool {
public:
  ThreadPool( int n );
  ~ThreadPool();

  void addTask( Task* nt );
  void finish();

private:
    std::vector<PoolWorkerThread*> threads;
    WorkQueue workQueue;
    bool finishing;
};

#endif
