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
  void MarkActive();
  void UnmarkActive();
  void WaitAllDone();
  bool IsSyncEvent();

  // Per-thread data
  evbuf_t*  evbuffer;    // Event buffer read from file
  Decoder   evdata;      // Decoded data
  detlst_t  detectors;   // Detectors with private event-by-event data
  varlst_t  variables;   // Interface to analysis results
  voutp_t   outvars;     // Output definitions
  int       nev;         // Event number given to this thread
  int       iseq;        // Event sequence number
  int       id;          // This thread's ID
  bool      is_init;     // Init() called successfully
  bool      is_active;

  static const int INIT_EVSIZE = 1024;

  struct SeqLess : public std::binary_function<Context*, Context*, bool>
  {
    bool operator() ( const Context* a, const Context* b ) const
    {
      assert( a && b );
      return (a->iseq < b->iseq);
    }
  };

private:
  static std::mutex fgMutex;
  static std::condition_variable fgAllDone;
  static int fgNactive;

  Context& operator=( const Context& rhs );
};

#endif
