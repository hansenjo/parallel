#include "ThreadPool.hpp"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>

using namespace std;
using namespace ThreadUtil;

// stdout is a shared resource, so protect it with a mutex
static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;


// Thanks to the reusable thread class implementing threads is
// simple and free of pthread api usage.
template< typename Data_t>
class AnalysisThread : public Thread<Data_t>
{
public:
  AnalysisThread( WorkQueue<Data_t>& wq, WorkQueue<Data_t>& fq )
    : Thread<Data_t>(wq,fq), fSeed(pthread_self())
  {
    srand(fSeed);
  }
protected:
  virtual void run()
  {
    while (Data_t* data = this->fWorkQueue.next()) {
      pthread_mutex_lock(&console_mutex);
      cout << "Thread " << pthread_self()
	   << ", data = " << setfill('0') << setw(5) << *data << endl;
      pthread_mutex_unlock(&console_mutex);
      usleep((unsigned int)((float)rand_r(&fSeed)/(float)RAND_MAX*20000));
      this->fFreeQueue.add(data);
    }
  }
private:
  unsigned int fSeed;
};

int main( int /* argc */, char** /* argv */ )
{
  const size_t NDATA = 64;

  typedef int thread_data_t;

  ThreadPool<AnalysisThread,thread_data_t> pool(32);

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

  return 0;
}
