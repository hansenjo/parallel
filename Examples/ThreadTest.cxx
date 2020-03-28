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

template <typename Data_t>
class AnalysisWorker
{
public:
  AnalysisWorker() : fGen(rd()), fRand(1,20) {}
  void run( ThreadPool<Data_t>* pool )
  {
    while( Data_t* data = pool->GetWorkQueue().next() ) {
#ifdef DEBUG
      console_mutex.lock();
      cout << "Thread " << pthread_self()
           << ", data = " << setw(5) << *data << endl << flush;
      console_mutex.unlock();
#endif
      int ms = fRand(fGen);
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
      *data *= -10;
      pool->GetResultQueue().add(data);
    }
  }
private:
  minstd_rand fGen;
  uniform_int_distribution<int> fRand;
};

template <typename Data_t>
class OutputWorker
{
public:
  explicit OutputWorker( WorkQueue<Data_t>& freeQueue ) : fFreeQueue( freeQueue) {}

  void run( ThreadPool<Data_t>* pool )
  {
    while( Data_t* data = pool->GetResultQueue().next() ) {
      console_mutex.lock();
      cout << "data = " << setw(5) << *data << endl << flush;
      console_mutex.unlock();
      assert(*data <= 0);
      fFreeQueue.add(data);
    }
    console_mutex.lock();
    cout << "Output thread terminating" << endl;
    console_mutex.unlock();
  }
private:
  WorkQueue<Data_t>& fFreeQueue;
};


int main( int /* argc */, char** /* argv */ )
{
  const size_t NTHREADS = 16;

  using thread_data_t = int;

  // Set up an array of reusable context buffers and add them to
  // the free queue
  thread_data_t data[NTHREADS];
  WorkQueue<thread_data_t> freeQueue;
  for(int& item : data) {
    freeQueue.add(&item);
  }
  AnalysisWorker<thread_data_t> analysisWorker;
  // Set up the pool of worker threads
  ThreadPool<thread_data_t> pool(NTHREADS,analysisWorker);
  // Set up and start the output queue. It takes processed items from
  // the pool's result queue, prints them, and puts them back into the
  // free queue
  OutputWorker<thread_data_t> outputWorker( freeQueue);
  std::thread outp(&OutputWorker<thread_data_t>::run, outputWorker, &pool);

  // Add work
  for( size_t i = 0; i < 10000; ++i ) {
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
