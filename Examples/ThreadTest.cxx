#include "ThreadPool.hpp"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>

using namespace std;

// stdout is a shared resource, so protect it with a mutex
static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;


// Thanks to the reusable thread class implementing threads is
// simple and free of pthread api usage.
template< typename Data_t>
class AnalysisThread : public Thread<Data_t>
{
public:
  AnalysisThread(WorkQueue<Data_t>& _work_queue, WorkQueue<Data_t>& _free_queue)
    : Thread<Data_t>(_work_queue, _free_queue), seed(pthread_self())
  {
    srand(seed);
  }
protected:
  virtual void run()
  {
    while (Data_t* data = this->work_queue.next()) {
      pthread_mutex_lock(&console_mutex);
      cout << "Thread " << pthread_self()
	   << ", data = " << setfill('0') << setw(5) << *data << endl;
      pthread_mutex_unlock(&console_mutex);
      usleep((unsigned int)((float)rand_r(&seed)/(float)RAND_MAX*20000));
      this->free_queue.add(data);
    }
  }
private:
  unsigned int seed;
};

int main( int /* argc */, char** /* argv */ )
{
  const size_t NDATA = 64;

  ThreadPool<AnalysisThread,int> pool(32);

  // Set up a pool of reusable context buffers
  int data[NDATA];
  for( size_t i = 0; i < NDATA; ++i ) {
    pool.addFreeData(&data[i]);
  }

  // Add work
  for( size_t i = 0; i < 10000; ++i ) {
    int* arg = (int*)pool.nextFree();
    *arg = i;
    pool.Process(arg);
  }

  return 0;
}
