#include "ThreadPool.hpp"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>
#include <ctime>

using namespace std;
using namespace ThreadUtil;

// stdout is a shared resource, so protect it with a mutex
//static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;


// Thanks to the reusable thread class implementing threads is
// simple and free of pthread api usage.
template< typename Data_t>
class AnalysisThread : public PoolWorkerThread<Data_t>
{
public:
  AnalysisThread( WorkQueue<Data_t>& wq, WorkQueue<Data_t>& rq,
                  const void* /* cfg */ )
    : PoolWorkerThread<Data_t>(wq,rq), fSeed(time(0))
  {
    srand(fSeed);
  }
protected:
  virtual void run()
  {
    while( Data_t* data = this->fWorkQueue.next() ) {
//      pthread_mutex_lock(&console_mutex);
//      cout << "Thread " << pthread_self()
//           << ", data = " << setw(5) << *data << endl;
//      pthread_mutex_unlock(&console_mutex);
      usleep((unsigned int)((float)rand_r(&fSeed)/(float)RAND_MAX*20000));
      *data *= -1;
      this->fResultQueue.add(data);
    }
  }
private:
  unsigned int fSeed;
};

template <typename Pool_t, typename Data_t>
class OutputThread : public Thread
{
public:
  explicit OutputThread( Pool_t& pool, WorkQueue<Data_t>& freeQueue )
          : fPool(pool), fFreeQueue(freeQueue) {}

protected:
  virtual void run()
  {
    while( Data_t* data = fPool.nextResult() ) {
      cout << "data = " << setw(5) << *data << endl;
      fFreeQueue.add(data);
    }
  }
private:
  Pool_t& fPool;
  WorkQueue<Data_t>& fFreeQueue;
};


int main( int /* argc */, char** /* argv */ )
{
  const size_t NTHREADS = 16;

  typedef int thread_data_t;
  typedef ThreadPool<AnalysisThread,thread_data_t> thread_pool_t;

  // Set up an array of reusable context buffers and add them to
  // the free queue
  thread_data_t data[NTHREADS];
  WorkQueue<thread_data_t> freeQueue;
  for(int& item : data) {
    freeQueue.add(&item);
  }
  // Set up the pool of worker threads
  thread_pool_t pool(NTHREADS);
  // Set up and start the output queue. It takes processed items from
  // the pool's result queue, prints them, and puts them back into the
  // free queue
  OutputThread<thread_pool_t,thread_data_t> outp(pool, freeQueue);
  outp.start();

  // Add work
  for( size_t i = 0; i < 1000; ++i ) {
    thread_data_t* datap = freeQueue.next();
    *datap = i;
    pool.Process(datap);
    // Note: Process() fills the pool's result queue and hence triggers
    // action in the output queue
  }

  pool.finish();
  pool.GetResultQueue().add(0);
  outp.join();

  return 0;
}
