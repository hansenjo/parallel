// Prototype parallel processing analyzer

#include "Podd.h"
#include "DataFile.h"
//#include "Decoder.h"
#include "DetectorTypeA.h"
#include "DetectorTypeB.h"
#include "Output.h"
#include "Util.h"
#include "ThreadPool.hpp"
#include "Context.h"

#include <iostream>
#include <climits>
#include <unistd.h>
#include <algorithm>  // for std::swap
#include <map>
#include <memory>
#include <thread>
#include <chrono>
#include <random>

// For output module
#include <fstream>
//#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

//#define OUTPUT_POOL

using namespace std;
using namespace ThreadUtil;
using namespace boost::iostreams;

// Definitions of global items declared in Podd.h

int debug = 0;

// Shared configuration data
static string prgname;
static int compress_output = 0;
static int delay_us = 0;
static bool order_events = false;
static bool allow_sync_events = false;
static random_device rd;

// Thread-safe random number generator
int intRand(int min, int max) {
  thread_local minstd_rand generator(rd());
  uniform_int_distribution<int> distribution(min, max);
  return distribution(generator);
}

template<typename Context_t>
class AnalysisWorker {
public:
  void run( QueuingThreadPool<Context_t>* pool ) {
    while( auto ctxPtr = pool->pop_work() ) {
      Context_t& ctx = *ctxPtr;

      // Process all defined analysis objects
      int status = ctx.evdata.Load(ctx.evbuffer);
      if( status != 0 ) {
        cerr << "Decoding error = " << status
             << " at event " << ctx.nev << endl;
        goto skip;
      }
      for( auto det : ctx.detectors ) {
        det->Clear();
        if( det->Decode(ctx.evdata) != 0 )
          goto skip;
        if( det->Analyze() != 0 )
          goto skip;
      }

      // If requested, add random delay
      if( delay_us > 0 ) {
        int us = intRand(0, delay_us);
        std::this_thread::sleep_for(std::chrono::microseconds(2 * us));
      }

     skip: //TODO: add error status to context, let output skip bad results
      pool->push_result( std::move(ctxPtr) );
    }
  }
};

template<typename Context_t>
class OutputWorker {
private:
  // Data shared between all output threads
  struct SharedData {
    SharedData() : fHeaderWritten(false), fLastWritten(0) {}
    ~SharedData() { close(); }
    int open( const string& odat_file ) {
      outp.open(odat_file, ios::out | ios::trunc | ios::binary);
      if( !outp )
        return 1;
      if( compress_output > 0 )
        outs.push(gzip_compressor());
      outs.push(outp);
      return 0;
    }
    void close() {
      outs.reset();
      outp.close();
    }
    mutex output_mutex;
    ofstream outp;
    ostrm_t outs;
    bool fHeaderWritten;
    size_t fLastWritten;
  };

  // Singleton shared data blob
  static inline SharedData fShared {};

  // Queue for finished contexts
  ConcurrentQueue<Context_t>& fFreeQueue;
  // Temporary storage for event ordering
  std::map<size_t, std::unique_ptr<Context_t>> fBuffer;

public:
  OutputWorker( const string& odat_file, ConcurrentQueue<Context_t>& freeQueue )
          : fFreeQueue(freeQueue) {
    // Open output file and set up filter chain
    if( fShared.open(odat_file) != 0 ) {
      cerr << "Error opening output data file " << odat_file << endl;
      return; // TODO: throw exception
    }
  }

  OutputWorker( const OutputWorker& rhs )
          : fFreeQueue(rhs.fFreeQueue), fBuffer{} {
    // Copy constructor. Called when used in std::thread
  }

  ~OutputWorker() = default;

  void run( QueuingThreadPool<Context_t>* pool ) {
    while( auto ctxPtr = pool->pop_result() ) {
#ifdef EVTORDER
      Context_t& ctx = *ctxPtr;
#endif
      ofstream& outp = fShared.outp;
      ostrm_t& outs = fShared.outs;

      std::lock_guard output_lock(fShared.output_mutex);
      if( !outp.good() || !outs.good() )
        // TODO: handle errors properly
        goto skip;

      if( !fShared.fHeaderWritten ) {
        WriteHeader(outs, ctxPtr.get());
        fShared.fHeaderWritten = true;
      }
#ifdef EVTORDER
      if( order_events ) {
        // Wait for next event in sequence before writing
        // FIXME: I don't think this works with > 1 thread
        auto& last_written = fShared.fLastWritten;
        if( ctx.iseq == last_written + 1 ) {
          WriteEvent(outs, ctxPtr.get());
          ++last_written;
          ctx.UnmarkActive();
          fFreeQueue.push(std::move(ctxPtr));
          // Check if some or all of the buffer can be written now, too
          for( auto it = fBuffer.begin(), jt = it;
               it != fBuffer.end() && (*it).first == last_written + 1; it = jt ) {
            ++jt;
            auto bufCtxPtr = std::move( (*it).second );
            WriteEvent(outs, bufCtxPtr.get());
            ++last_written;
            bufCtxPtr->UnmarkActive();
            fFreeQueue.push( std::move(bufCtxPtr) );
            fBuffer.erase(it);
          }
        } else {
          // Buffer out-of-order events, sorted by iseq
          fBuffer.emplace(ctx.iseq, std::move(ctxPtr));
          //TODO: error check
          //TODO: deal with skipped events!
        }
      } else
#endif
      {
        WriteEvent(outs, ctxPtr.get());
       skip:
#ifdef EVTORDER
        ctx.UnmarkActive();
#endif
        fFreeQueue.push( std::move(ctxPtr) );
      }
    }
  }

private:
  void WriteEvent( ostrm_t& os, Context_t* ctx, bool do_header = false ) {
    // Write output file data (or header names)
    for( auto var : ctx->outvars ) {
      var->write(os, do_header);
    }
    if( debug > 0 && !do_header )
      cout << "Wrote nev = " << ctx->nev << endl;
  }
  void WriteHeader( ostrm_t& os, Context_t* ctx ) {
    // Write output file header
    // <N = number of variables> N*<variable type> N*<variable name C-string>
    // where
    //  <variable type> = TTTNNNNN,
    // with
    //  TTT   = type (0=int, 1=unsigned, 2=float/double, 3=C-string)
    //  NNNNN = number of bytes
    uint32_t nvars = ctx->outvars.size();
    os.write(reinterpret_cast<const char*>(&nvars), sizeof(nvars));
    for( auto it = ctx->outvars.begin();
         it != ctx->outvars.end(); ++it ) {
      OutputElement* var = *it;
      char type = var->GetType();
      os.write(&type, sizeof(type));
    }
    WriteEvent(os, ctx, true);
  }
};

static void default_names( string infile, string& odef, string& odat ) {
  // If not given, set defaults for odef and output files
  if( infile.empty() )
    return;
  string::size_type pos = infile.rfind('.');
  if( pos != string::npos )
    infile.erase(pos);
  // Ignore any directory component in the file name
  pos = infile.rfind('/');
  if( pos != string::npos && pos + 1 < infile.size() )
    infile.erase(0, pos + 1);
  if( odef.empty() )
    odef = infile + ".odef";
  if( odat.empty() )
    odat = infile + ".odat";
}

static void usage() {
  cerr << "Usage: " << prgname << " [options] input_file.dat" << endl
       << "where options are:" << endl
       << " [ -c odef_file ]\tread output definitions from odef_file"
       << " (default = input_file.odef)" << endl
       << " [ -o outfile ]\t\twrite output to odat_file"
       << " (default = input_file.odat)" << endl
       << " [ -d debug_level ]\tset debug level" << endl
       << " [ -n nev_max ]\t\tset max number of events" << endl
       << " [ -t nthreads ]\tcreate at most nthreads (default = n_cpus)" << endl
       << " [ -y us ]\t\tAdd us microseconds average random delay per event" << endl
#ifdef EVTORDER
       << " [ -e (sync|strict) ]\tPreserve event order" << endl
#endif
       << " [ -m ]\t\t\tMark progress" << endl
       << " [ -z ]\t\t\tCompress output with gzip" << endl
       << " [ -h ]\t\t\tPrint this help message" << endl;
  exit(255);
}

int main( int argc, char* const* argv )
{
  // Parse command line
  unsigned long nev_max = ULONG_MAX;
  int opt;
  bool mark = false;
  string input_file, odef_file, odat_file;
  unsigned int nthreads = 0;

  prgname = argv[0];
  if( prgname.size() >= 2 && prgname.substr(0, 2) == "./" )
    prgname.erase(0, 2);

  while( (opt = getopt(argc, argv, "c:d:n:o:t:y:e:zmh")) != -1 ) {
    switch( opt ) {
      case 'c':
        odef_file = optarg;
        break;
      case 'd':
        debug = atoi(optarg);
        break;
      case 'n':
        nev_max = atoi(optarg);
        break;
      case 'o':
        odat_file = optarg;
        break;
      case 't': {
        int i =  atoi(optarg);
        if( i > 0 )
          nthreads = i;
        else
          cerr << "Invalid number of threads specified: " << i
               << ", assuming 1";
        }
        break;
      case 'y':
        delay_us = atoi(optarg);
        break;
#ifdef EVTORDER
      case 'e':
        if( optarg && !strcmp(optarg, "strict") ) {
          order_events = true;
          allow_sync_events = true;
        } else if( optarg && !strcmp(optarg, "sync") ) {
          allow_sync_events = true;
        } else {
          usage();
        }
        break;
#endif
      case 'z':
        compress_output = 1;
        break;
      case 'm':
        mark = true;
        break;
      case 'h':
      default:
        usage();
        break;
    }
  }
  if( optind >= argc ) {
    cerr << "Input file name missing" << endl;
    usage();
  }
  input_file = argv[optind];

  default_names(input_file, odef_file, odat_file);

  if( debug > 0 ) {
    cout << "input_file = " << input_file << endl;
    cout << "odef_file = " << odef_file << endl;
    cout << "odat_file = " << odat_file << endl;
    cout << "compress_output = " << compress_output << endl;
    cout << "order_events = " << order_events << endl;
    cout << "allow_sync_events = " << allow_sync_events << endl;
  }
  // Open input
  DataFile inp(input_file.c_str());
  if( inp.Open() )
    return 2;

  detlst_t gDets;
  // Set up analysis objects
  gDets.push_back(new DetectorTypeA("detA", 1));
  gDets.push_back(new DetectorTypeB("detB", 2));

  // if( debug > 1 )
  //   PrintVarList(gVars);

  // Set up thread contexts. Copy analysis objects.
  unsigned int ncpu = GetThreadCount();
  if( nthreads == 0 || nthreads > ncpu - 1 )
    nthreads = (ncpu > 1) ? ncpu - 1 : 1;
  if( debug > 0 )
    cout << "Initializing " << nthreads << " analysis threads" << endl;

  using Queue_t = ConcurrentQueue<Context>;
  Queue_t freeQueue;
  for( unsigned int i=0; i<nthreads; ++i ) {
    // Deep copy of container objects
    std::unique_ptr<Context> ctx(new Context);
    CopyContainer(gDets, ctx->detectors);
    freeQueue.push( std::move(ctx) );
  }

  // Configure output
  if( compress_output > 0 && odat_file.size() > 3
      && odat_file.substr(odat_file.size() - 3) != ".gz" )
    odat_file.append(".gz");

  // Set up nthreads analysis threads. Finished Contexts go into the output queue
  AnalysisWorker<Context> analysisWorker;
  QueuingThreadPool<Context> pool(nthreads, analysisWorker);

  // Set up output thread(s). Finished Contexts go back into freeQueue
#ifdef OUTPUT_POOL
  QueuingThreadPool<Context> out_pool( 1, outputWorker );
#else
  // Single output thread
  std::thread output(&OutputWorker<Context>::run,
                     OutputWorker<Context>(odat_file, freeQueue), &pool);
#endif

#ifdef EVTORDER
  bool doing_sync = false;
#endif
  size_t nev = 0;
  if( debug > 0 )
    cout << "Starting event loop, nev_max = " << nev_max << endl;

  // Loop: Read one event and hand it off to an idle thread
  while( inp.ReadEvent() == 0 && nev < nev_max ) {
    ++nev;
    if( mark && (nev % 1000) == 0 )
      cout << nev << endl;
    // Main processing
    if( debug > 1 )
      cout << "Event " << nev << endl;

    auto ctxPtr = freeQueue.next();
    Context& ctx = *ctxPtr;
    // Init if necessary
    //TODO: split up Init:
    // (1) Read database and all other related things, do before cloning
    //     detectors
    // (2) DefineVariables: do in threads
    if( !ctx.is_init ) {
      if( ctx.Init(odef_file.c_str()) != 0 )
        break;
    }

    swap(ctx.evbuffer, inp.GetEvBuffer());
    ctx.nev = nev;
    // Sequence number for event ordering. These must be consecutive
    ctx.iseq = nev;

#ifdef EVTORDER
    // Synchronize the event stream at sync events (e.g. scalers).
    // All events before sync events will be processed, followed by
    // the sync event(s), then normal processing resumes.
    if( allow_sync_events && (ctx.IsSyncEvent() || doing_sync) ) {
      ctx.WaitAllDone();
      doing_sync = ctx.IsSyncEvent();
    }
    ctx.MarkActive();
#endif
    pool.push_work(std::move(ctxPtr));
  }
  if( debug > 0 ) {
    cout << "Normal end of file" << endl;
    cout << "Read " << nev << " events" << endl;
  }

  inp.Close();

  // Terminate worker threads
  pool.finish();
#ifdef OUTPUT_POOL
  // Terminate output threads
  out_pool.finish();
#else
  // Terminate single output thread
  pool.push_result(nullptr);
  output.join();
#endif

  DeleteContainer(gDets);

  return 0;
}
