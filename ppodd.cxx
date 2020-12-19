// Prototype parallel processing analyzer

#include "Podd.h"
#include "DataFile.h"
//#include "Decoder.h"
#include "DetectorTypeA.h"
#include "DetectorTypeB.h"
#include "DetectorTypeC.h"
#include "Output.h"
#include "Util.h"
#include "ThreadPool.hpp"
#include "Context.h"
#include "Database.h"

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <algorithm>  // for std::swap
#include <map>
#include <memory>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <stdexcept>

// For output module
#include <fstream>
//#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

//#define OUTPUT_POOL

using namespace std;
using namespace ThreadUtil;
using namespace boost::iostreams;
using HighResClock = std::chrono::high_resolution_clock;
using ClockTime_t  = std::chrono::duration<double, std::milli>;

// Definitions of global items declared in Podd.h

int debug = 0;
Config cfg;

// Shared configuration data
static string prgname;
static int compress_output = 0;
static int delay_us = 0;
static bool order_events = false;
static bool allow_sync_events = false;

static mutex time_sum_mutex;
static ClockTime_t analysis_realtime_sum;
static ClockTime_t output_realtime_sum;

template<typename Context_t>
class AnalysisWorker {
private:
  ClockTime_t m_time_spent;

public:
  AnalysisWorker() : m_time_spent{} {}

  void run( QueuingThreadPool<Context_t>* pool ) {
    while( auto ctxPtr = pool->pop_work() ) {
      auto start = HighResClock::now();
      Context_t& ctx = *ctxPtr;

      // Process all defined analysis objects
      if( int status = ctx.evdata.Load(ctx.evbuffer.get()) ) {
        cerr << "Decoding error = " << status
             << " at event " << ctx.nev << endl;
        goto skip;
      }
      for( auto& det : ctx.detectors ) {
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
      auto stop = HighResClock::now();
      m_time_spent += stop-start;
      pool->push_result( std::move(ctxPtr) );
    }

    std::lock_guard time_lock(time_sum_mutex);
    analysis_realtime_sum += m_time_spent;
  }
};

template<typename Context_t>
class OutputWorker {
private:
  // Queue for finished contexts
  ConcurrentQueue<Context_t>& fFreeQueue;
  // Temporary storage for event ordering
  std::map<size_t, std::unique_ptr<Context_t>> fBuffer;
  ClockTime_t m_time_spent;

  // Data shared between all output threads
  struct SharedData {
    SharedData() : fLastWritten(0), fHeaderWritten(false) {}
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
    size_t fLastWritten;
    bool fHeaderWritten;
  } __attribute__((aligned(128)));

  // Singleton shared data blob
  static inline SharedData fShared {};

  void WriteEvent( ostrm_t& os, Context_t* ctx, bool do_header = false ) {
    // Write output file data (or header names)
    for( auto& var : ctx->outvars ) {
      var->write(os, do_header);
    }
    if( debug > 1 && !do_header )
      cout << "Wrote nev = " << ctx->nev << endl;
  }
  void WriteHeader( ostrm_t& os, Context_t* ctx ) {
    // Write output file header
    // <N = number of variables> N*<variable typ£££e> N*<variable name C-string>
    // where
    //  <variable type> = TTTNNNNN,
    // with
    //  TTT   = type (0=int, 1=unsigned, 2=float/double, 3=C-string)
    //  NNNNN = number of bytes
    uint32_t nvars = ctx->outvars.size();
    os.write(reinterpret_cast<const char*>(&nvars), sizeof(nvars));
    for( auto& var : ctx->outvars ) {
      char type = var->GetType();
      os.write(&type, sizeof(type));
    }
    WriteEvent(os, ctx, true);
  }

public:
  OutputWorker( const string& odat_file, ConcurrentQueue<Context_t>& freeQueue )
          : fFreeQueue(freeQueue), fBuffer{}, m_time_spent{} {
    // Open output file and set up filter chain
    if( fShared.open(odat_file) != 0 ) {
      cerr << "Error opening output data file " << odat_file << endl;
      return; // TODO: throw exception
    }
  }

  OutputWorker( const OutputWorker& rhs )
          : fFreeQueue(rhs.fFreeQueue), fBuffer{}, m_time_spent{} {
    // Copy constructor. Called when used in std::thread
  }

  ~OutputWorker() = default;

  void run( QueuingThreadPool<Context_t>* pool ) {
    while( auto ctxPtr = pool->pop_result() ) {
      auto start = HighResClock::now();
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
        if( auto& last_written = fShared.fLastWritten;
                ctx.iseq == last_written + 1 ) {
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
        if( order_events )
          ctx.UnmarkActive();
#endif
        auto stop = HighResClock::now();
        m_time_spent += stop-start;
        fFreeQueue.push( std::move(ctxPtr) );
      }
    }
    std::lock_guard time_lock(time_sum_mutex);
    output_realtime_sum += m_time_spent;
  }
};

// Set any unset filenames to defaults (= input file name + extension)
void Config::default_names() {
  // If not given, set defaults for odef, db and output files
  if( input_file.empty() ||
      !(odef_file.empty() || output_file.empty() || db_file.empty()) )
    return;
  string infile{ input_file };
  // Drop input file filename extension
  string::size_type pos = infile.rfind('.');
  if( pos != string::npos )
    infile.erase(pos);
  // Ignore any directory component of the input file name
  pos = infile.rfind('/');
  if( pos != string::npos && pos+1 < infile.size() )
    infile.erase(0, pos+1);
  if( odef_file.empty() )
    odef_file = infile + ".odef";
  if( output_file.empty() )
    output_file = infile + ".out";
  if( db_file.empty() )
    db_file = infile + ".db";
}

static void usage() {
  cerr << "Usage: " << prgname << " [options] input_file.dat" << endl
       << "where options are:" << endl
       << " [ -c odef_file ]\tread output definitions from odef_file"
       << " (default = input_file.odef)" << endl
       << " [ -o outfile ]\t\twrite output to output_file"
       << " (default = input_file.odat)" << endl
       << " [ -b db_file ]\t\tuse database file db_file"
       << " (default = input_file.db)" << endl
       << " [ -d debug_level ]\tset debug level" << endl
       << " [ -n nev_max ]\t\tset max number of events" << endl
       << " [ -j nthreads ]\tcreate at most nthreads (default = n_cpus)" << endl
       << " [ -y us ]\t\tAdd us microseconds average random delay per event" << endl
#ifdef EVTORDER
       << " [ -e (sync|strict) ]\tPreserve event order" << endl
#endif
       << " [ -m interval ]\tMark progress at given intervals" << endl
       << " [ -z ]\t\t\tCompress output with gzip" << endl
       << " [ -h ]\t\t\tPrint this help message" << endl;
  exit(255);
}

// Parse command line
void get_args(int argc, char* const* argv )
{
  prgname = argv[0];
  // Drop path form program name
  if( string::size_type pos = prgname.rfind('/');
          pos != string::npos && pos+1 < prgname.length() )
    prgname.erase(0, pos+1);

  try {
    int opt;
    while( (opt = getopt(argc, argv, "b:c:d:n:o:j:y:e:m:zmh")) != -1 ) {
      switch( opt ) {
        case 'b':
          cfg.db_file = optarg;
          break;
        case 'c':
          cfg.odef_file = optarg;
          break;
        case 'd':
          debug = stoi(optarg);
          break;
        case 'n':
          cfg.nev_max = stoi(optarg);
          break;
        case 'o':
          cfg.output_file = optarg;
          break;
        case 'j': {
          int i = stoi(optarg);
          if( i > 0 )
            cfg.nthreads = i;
          else {
            cerr << "Invalid number of threads specified: " << i
                 << ", assuming 1";
            cfg.nthreads = 1;
          }
        }
          break;
        case 'y':
          delay_us = stoi(optarg);
          break;
#ifdef EVTORDER
          case 'e':
          if( !optarg ) usage();
          if( !strcmp(optarg, "strict") ) {
            order_events = true;
            allow_sync_events = true;
          } else if( !strcmp(optarg, "sync") ) {
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
          cfg.mark = stoi(optarg);
          break;
        case 'h':
        default:
          usage();
          break;
      }
    }
  }
  catch( const exception& e ) {
    cerr << "Error: " << e.what() << endl;
    usage();
  }
  if( optind >= argc ) {
    cerr << "Input file name missing" << endl;
    usage();
  }
  cfg.input_file = argv[optind];
  cfg.default_names();
}

static void mark_progress( size_t nev )
{
  if( cfg.mark != 0 ) {
    auto d = lldiv(nev,cfg.mark);
    if( d.rem == 0 ) {
      if( nev > cfg.mark )
        cout << "..";
      cout << nev << flush;
    }
  }
}

// Main program
int main( int argc, char* const* argv )
{
  get_args(argc, argv);

  if( debug > 0 ) {
    cout << "input_file  = " << cfg.input_file  << endl;
    cout << "odef_file   = " << cfg.odef_file   << endl;
    cout << "output_file = " << cfg.output_file << endl;
    cout << "compress_output   = " << compress_output   << endl;
    cout << "order_events      = " << order_events      << endl;
    cout << "allow_sync_events = " << allow_sync_events << endl;
  }

  // Read database, if any
  database.Open(cfg.db_file);
  if( auto sz = database.GetSize(); sz > 0 ) {
    cout << "Read " << sz << " parameters from database " << cfg.db_file << endl;
    if( debug > 0 )
      database.Print();
  }

  // Start timers
  timespec start_clock{}, stop_clock{}, clock_diff{};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_clock);
  auto start = HighResClock::now();
  auto init_start = HighResClock::now();

  // Set up analysis objects
  detlst_t gDets;
  gDets.push_back( make_unique<DetectorTypeA>("detA", 1));
  gDets.push_back( make_unique<DetectorTypeB>("detB", 2));
  gDets.push_back( make_unique<DetectorTypeC>("detC", 3));

  // Set up thread contexts. Copy analysis objects.
  unsigned int ncores = GetThreadCount(), nthreads = cfg.nthreads;
  if( nthreads > 2*ncores )
    nthreads = 2*ncores;
  if( nthreads == 0 )
    nthreads = (ncores > 1) ? ncores - 1 : 1;
  if( debug > 0 )
    cout << "Initializing " << nthreads << " analysis threads" << endl;

  using Queue_t = ConcurrentQueue<Context>;
  Queue_t freeQueue;
  for( unsigned int i=0; i<nthreads; ++i ) {
    // Make new context
    auto ctxPtr = make_unique<Context>();
    Context& ctx = *ctxPtr;
    // Clone detectors into each new context
    CopyContainer(gDets, ctx.detectors);
    // Init if necessary
    //TODO: split up Init:
    // (1) Read database and all other related things, do before cloning
    //     detectors
    // (2) DefineVariables: do in threads
    if( !ctx.is_init ) {
      if( ctx.Init() != 0 )
        // Die on failure to initialize (usually database read error)
        return 1;
    }
    freeQueue.push( std::move(ctxPtr) );
  }
  gDets.clear();  // No need to keep the prototype detector objects around

  // if( debug > 1 )
  //   PrintVarList(gVars);

  // Configure output
  if( compress_output > 0 && cfg.output_file.size() > 3
      && cfg.output_file.substr(cfg.output_file.size() - 3) != ".gz" )
    cfg.output_file.append(".gz");

  // Set up nthreads analysis threads. Finished Contexts go into the output queue
  AnalysisWorker<Context> analysisWorker;
  QueuingThreadPool<Context> pool(nthreads, analysisWorker);

  // Set up output thread(s). Finished Contexts go back into freeQueue
#ifdef OUTPUT_POOL
  QueuingThreadPool<Context> out_pool( 1, outputWorker );
#else
  // Single output thread
  std::thread output(&OutputWorker<Context>::run,
                     OutputWorker<Context>(cfg.output_file, freeQueue), &pool);
#endif

  ClockTime_t init_duration = HighResClock::now() - init_start;

#ifdef EVTORDER
  bool doing_sync = false;
#endif
  size_t nev = 0;
  if( debug > 0 )
    cout << "Starting event loop, nev_max = " << cfg.nev_max << endl;

  // Open input
  DataFile inp(cfg.input_file);
  if( inp.Open() )
    return 2;

  // Loop: Read one event and hand it off to an idle thread
  while( inp.ReadEvent() == 0 && nev < cfg.nev_max ) {
    ++nev;
    if( debug > 1 )
      cout << "Event " << nev << endl;
    else
      mark_progress(nev);
    // Main processing

    auto ctxPtr = freeQueue.next();
    Context& ctx = *ctxPtr;

    swap(ctx.evbuffer, inp.GetEvBuffer());
    ctx.nev = nev;

#ifdef EVTORDER
    // Sequence number for event ordering. These must be consecutive
    ctx.iseq = nev;

    // Synchronize the event stream at sync events (e.g. scalers).
    // All events before sync events will be processed, followed by
    // the sync event(s), then normal processing resumes.
    if( allow_sync_events && (ctx.IsSyncEvent() || doing_sync) ) {
      ctx.WaitAllDone();
      doing_sync = ctx.IsSyncEvent();
    }
    if( order_events )
      ctx.MarkActive();
#endif
    pool.push_work(std::move(ctxPtr));
  }
  if( cfg.mark != 0 && nev >= cfg.mark )
    cout << endl;
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

  // Total wall time
  ClockTime_t run_duration = HighResClock::now() - start;
  // Total CPU time
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_clock);
  timespec_diff( &stop_clock, &start_clock, &clock_diff );
  ClockTime_t cpu_usage( std::chrono::seconds(clock_diff.tv_sec) +
                         std::chrono::nanoseconds(clock_diff.tv_nsec) );
  cout << "Timing analysis:" << endl;
  cout << "Init:      " << init_duration.count()         << " ms" << endl;
  cout << "Analysis:  " << analysis_realtime_sum.count() << " ms" << endl;
  cout << "Output:    " << output_realtime_sum.count()   << " ms" << endl;
  cout << "Total CPU: " << cpu_usage.count()             << " ms" << endl;
  cout << "Real:      " << run_duration.count()          << " ms" << endl;
  return 0;
}
