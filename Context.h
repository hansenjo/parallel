// Per-thread analysis context

#ifndef PPODD_CONTEXT
#define PPODD_CONTEXT

#include "Podd.h"
#include "Decoder.h"
#include "Output.h"
#include "pthread.h"

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

private:
  static pthread_mutex_t fgMutex;
  static pthread_cond_t  fgAllDone;
  static int fgNactive;
  static int fgNctx;

  static void StaticInit();
  static void StaticCleanup();

  Context& operator=( const Context& rhs );
};

#endif
