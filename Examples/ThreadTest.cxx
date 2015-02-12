#include "ThreadPool.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "unistd.h"

using namespace std;

// stdout is a shared resource, so protect it with a mutex
static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;


// Thanks to the reusable thread class implementing threads is
// simple and free of pthread api usage.
class AnalysisThread : public Thread
{
public:
  AnalysisThread(WorkQueue& _work_queue, WorkQueue& _free_queue) :
    Thread(_work_queue, _free_queue) {}

protected:
  virtual void run()
  {
    while (void* arg = work_queue.next()) {
      int* data = (int*)arg;
      pthread_mutex_lock(&console_mutex);
      cout << "Thread " << pthread_self()
	   << ", data = " << setfill('0') << setw(5) << data[0] << endl;
      pthread_mutex_unlock(&console_mutex);
      usleep((unsigned int)((float)rand()/(float)RAND_MAX*20000));
      free_queue.add(arg);
    }
  }
};

int main( int /* argc */, char** /* argv */ )
{
  const size_t NDATA = 20;

  ThreadPool<AnalysisThread> pool(5);

  // Set up a pool of reusable data buffers
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
