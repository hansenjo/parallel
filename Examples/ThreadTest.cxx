#include "ThreadPool.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>
#include <thread>

using namespace std;
using namespace ThreadUtil;

static mutex console_mutex;
static random_device rd;

using namespace std;

// Thread-safe random number generator
int intRand(int min, int max) {
  thread_local minstd_rand generator(rd());
  uniform_int_distribution<int> distribution(min, max);
  return distribution(generator);
}

template<typename Data_t>
class AnalysisWorker {
public:
  void run( QueuingThreadPool<Data_t>* pool ) {
    while( auto ptr = pool->pop_work() ) {
#ifdef DEBUG
      console_mutex.lock();
      cout << "Thread " << pthread_self()
           << ", data = " << setw(5) << *data << endl << flush;
      console_mutex.unlock();
#endif
      int ms = intRand(1,20);
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
      Data_t& data = *ptr;
      data *= -10;
      pool->push_result(std::move(ptr));
    }
  }
};

template<typename Data_t>
class OutputWorker {
public:
  OutputWorker( WorkQueue<Data_t>& resultQueue, WorkQueue<Data_t>& freeQueue )
          : fResultQueue(resultQueue), fFreeQueue(freeQueue) {}

  void run() {
    while( auto ptr = fResultQueue.next() ) {
      const Data_t& data = *ptr;
      console_mutex.lock();
      cout << "data = " << setw(5) << data << endl << flush;
      console_mutex.unlock();
      assert(data <= 0);
      fFreeQueue.push(std::move(ptr));
    }
    console_mutex.lock();
    cout << "Output thread terminating" << endl;
    console_mutex.unlock();
  }
private:
  WorkQueue<Data_t>& fResultQueue;
  WorkQueue<Data_t>& fFreeQueue;
};


int main( int /* argc */, const char*[] /* argv */ )
{
  const size_t NTHREADS = 16;

  // Data type to be processed in the threads
  using thread_data_t = int;

  // Allocate data objects and add them to the free queue
  WorkQueue<thread_data_t> freeQueue;
  for( size_t i=0; i<NTHREADS; ++i ) {
    std::unique_ptr<thread_data_t> item(new thread_data_t);
    freeQueue.push(std::move(item));
  }
  // Set up the pool of worker threads
  QueuingThreadPool pool(NTHREADS, AnalysisWorker<thread_data_t>());

  // Set up and start the output queue. It takes processed items from
  // the pool's result queue, prints them, and puts them back into the
  // free queue
  std::thread outp(&OutputWorker<thread_data_t>::run,
                   OutputWorker<thread_data_t>(pool.GetResultQueue(), freeQueue));

  // Add work
  for( size_t i = 0; i < 10000; ++i ) {
    auto ptr = freeQueue.next();
    thread_data_t& data = *ptr;
    data = i;
    pool.push_work(std::move(ptr));
    // push_work() wakes up the analysis workers. The workers then fill the
    // pool's result queue. This in turn triggers action in the output queue.
  }

  pool.finish();
  pool.push_result(nullptr);  // Terminate output thread
  outp.join();

  return 0;
}
