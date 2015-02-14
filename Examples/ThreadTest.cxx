#include "ThreadPool.hpp"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>

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
  AnalysisThread( WorkQueue<Data_t>& wq, WorkQueue<Data_t>& fq,
		  WorkQueue<Data_t>& rq )
    : PoolWorkerThread<Data_t>(wq,fq,rq), fSeed(pthread_self())
  {
    srand(fSeed);
  }
protected:
  virtual void run()
  {
    while( Data_t* data = this->fWorkQueue.next() ) {
      // pthread_mutex_lock(&console_mutex);
      // cout << "Thread " << pthread_self()
      // 	   << ", data = " << setfill('0') << setw(5) << *data << endl;
      // pthread_mutex_unlock(&console_mutex);
      usleep((unsigned int)((float)rand_r(&fSeed)/(float)RAND_MAX*20000));
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
  OutputThread( Pool_t& pool ) : fPool(pool) {}

protected:
  virtual void run()
  {
    while( Data_t* data = fPool.nextResult() ) {
      cout << "data = " << setfill('0') << setw(5) << *data << endl;
      fPool.addFreeData(data);
    }
  }
private:
  Pool_t& fPool;
};


int main( int /* argc */, char** /* argv */ )
{
  const size_t NDATA = 64;

  typedef int thread_data_t;
  typedef ThreadPool<AnalysisThread,thread_data_t> thread_pool_t;

  thread_pool_t pool(16);
  OutputThread<thread_pool_t,thread_data_t> outp(pool);
  outp.start();

  // Set up a pool of reusable context buffers
  thread_data_t data[NDATA];
  for( size_t i = 0; i < NDATA; ++i ) {
    pool.addFreeData(&data[i]);
  }

  // Add work
  for( size_t i = 0; i < 10000; ++i ) {
    thread_data_t* datap = pool.nextFree();
    *datap = i;
    pool.Process(datap);
  }

  pool.finish();
  pool.addResult(0);
  outp.join();

  return 0;
}
