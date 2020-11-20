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

class Context {
public:
  Context();
  Context( const Context& context );
  ~Context();

  int Init( const char* odef_file );
#ifdef EVTORDER
  void MarkActive();
  void UnmarkActive();
  void WaitAllDone();
  bool IsSyncEvent();
#endif

  // Per-thread data
  evbuf_t*  evbuffer;    // Event buffer read from file
  Decoder   evdata;      // Decoded data
  detlst_t  detectors;   // Detectors with private event-by-event data
  varlst_t  variables;   // Interface to analysis results
  voutp_t   outvars;     // Output definitions
  size_t    nev;         // Event number given to this thread
  size_t    iseq;        // Event sequence number
  //int       id;          // This thread's ID
  bool      is_init;     // Init() called successfully
  bool      is_active;

  static const int INIT_EVSIZE = 1024;

private:
#ifdef EVTORDER
  static std::mutex fgMutex;
  static std::condition_variable fgAllDone;
  static int fgNactive;
#endif

  Context& operator=( const Context& rhs );
};

#endif
