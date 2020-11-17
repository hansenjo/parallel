#include "ThreadPool.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>

using namespace std;
using namespace ThreadUtil;

// stdout is a shared resource, so protect it with a mutex
static mutex console_mutex;
static random_device rd;

template<typename Data_t>
class AnalysisWorker {
public:
  AnalysisWorker() : fGen(rd()), fRand(1, 20) {}
  void run( QueuingThreadPool<Data_t>* pool ) {
    while( std::unique_ptr<Data_t> data = pool->pop_work() ) {
#ifdef DEBUG
      console_mutex.lock();
      cout << "Thread " << pthread_self()
           << ", data = " << setw(5) << *data << endl << flush;
      console_mutex.unlock();
#endif
      int ms = fRand(fGen);
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
      *data *= -10;
      pool->push_result(std::move(data));
    }
  }
private:
  minstd_rand fGen;
  uniform_int_distribution<int> fRand;
};

template<typename Data_t>
class OutputWorker {
public:
  OutputWorker( WorkQueue<Data_t>& resultQueue, WorkQueue<Data_t>& freeQueue )
          : fResultQueue(resultQueue), fFreeQueue(freeQueue) {}

  void run() {
    while( std::unique_ptr<Data_t> data = fResultQueue.next() ) {
      console_mutex.lock();
      cout << "data = " << setw(5) << *data << endl << flush;
      console_mutex.unlock();
      assert(*data <= 0);
      fFreeQueue.push(std::move(data));
    }
    console_mutex.lock();
    cout << "Output thread terminating" << endl;
    console_mutex.unlock();
  }
private:
  WorkQueue<Data_t>& fResultQueue;
  WorkQueue<Data_t>& fFreeQueue;
};


int main( int /* argc */, char** /* argv */ )
{
  const size_t NTHREADS = 16;

  using thread_data_t = int;

  // Set up an array of reusable context buffers and add them to
  // the free queue
  WorkQueue<thread_data_t> freeQueue;
  for( size_t i=0; i<NTHREADS; ++i ) {
    std::unique_ptr<thread_data_t> item(new thread_data_t);
    freeQueue.push(std::move(item));
  }
  AnalysisWorker<thread_data_t> analysisWorker;
  // Set up the pool of worker threads
  QueuingThreadPool<thread_data_t> pool(NTHREADS, analysisWorker);
  // With template argument deduction, this works too, although it's less clear
  //QueuingThreadPool pool(NTHREADS, analysisWorker);
  // Set up and start the output queue. It takes processed items from
  // the pool's result queue, prints them, and puts them back into the
  // free queue
  std::thread outp(&OutputWorker<thread_data_t>::run,
                   OutputWorker<thread_data_t>(pool.GetResultQueue(), freeQueue));

  // Add work
  for( size_t i = 0; i < 10000; ++i ) {
    std::unique_ptr<thread_data_t> datap = freeQueue.next();
    *datap = i;
    pool.push_work(std::move(datap));
    // Note: push_work() fills the pool's result queue and hence triggers
    // action in the output queue
  }

  pool.finish();
  pool.push_result(nullptr);  // Terminate output thread
  outp.join();

  return 0;
}
