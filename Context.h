// Per-thread analysis context

#ifndef PPODD_CONTEXT
#define PPODD_CONTEXT

#include "Podd.h"
#include "Decoder.h"
#include "Output.h"
#include <thread>
#include <functional>
#include <cassert>
#include <memory>
#include <string>

class Context {
public:
  explicit Context( int id = 0 );
  Context( const Context& context ); //FIXME: but operator= is deleted?
  Context& operator=( const Context& rhs ) = delete;
  ~Context();

  int Init();
#ifdef EVTORDER
  void MarkActive();
  void UnmarkActive();
  void WaitAllDone();
  bool IsSyncEvent();
#endif

  // Per-thread data
#ifndef PPODD_TBB
  evbuf_ptr_t evbuffer;  // Event buffer read from file
#endif
  Decoder   evdata;      // Decoded data
  detlst_t  detectors;   // Detectors with private event-by-event data
  std::shared_ptr<varlst_t> variables;   // Interface to analysis results
  voutp_t   outvars;     // Output definitions
  size_t    nev{};       // Event number given to this thread
  size_t    iseq{};      // Event sequence number
  int       id{};        // This context's ID
  bool      is_init;     // Init() called successfully
  bool      is_active;   // Currently being processed in a worker thread
  ClockTime_t m_time_spent{}; // Analysis time sum

private:
#ifdef EVTORDER
  static std::mutex fgMutex;
  static std::condition_variable fgAllDone;
  static int fgNactive;
#endif
};

#endif
