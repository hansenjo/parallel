// Prototype parallel processing analyzer

#include "Podd.h"
#include "DataFile.h"
#include "DetectorTypeA.h"
#include "DetectorTypeB.h"
#include "DetectorTypeC.h"
#include "Output.h"
#include "Util.h"
#include "Context.h"
#include "Database.h"

#include <iostream>
#include <unistd.h>
#include <map>
#include <memory>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <numeric>

#include <oneapi/tbb/flow_graph.h>
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/concurrent_queue.h>

// For output module
#include <fstream>
//#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;
using namespace tbb;
using namespace tbb::flow;
using namespace boost::iostreams;
using HighResClock = std::chrono::high_resolution_clock;

// Definitions of global items declared in Podd.h

int debug = 0;
Config cfg;

// Shared configuration data
string prgname;
int compress_output = 0;
int delay_us = 0;
enum Mode { kUnordered, kPreserveSpecial, kOrdered };
Mode mode = kUnordered;

//-------------------------------------------------------------
class file_io_error : public std::runtime_error {
public:
  explicit file_io_error( const std::string& what_arg )
    : std::runtime_error(what_arg) {}
};

//-------------------------------------------------------------
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

//-------------------------------------------------------------
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
       << " [ -e (sync|strict) ]\tPreserve event order" << endl
       << " [ -m interval ]\tMark progress at given intervals" << endl
       << " [ -z ]\t\t\tCompress output with gzip" << endl
       << " [ -h ]\t\t\tPrint this help message" << endl;
  exit(255);
}

//-------------------------------------------------------------
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
        case 'e':
          if( !optarg ) usage();
          if( !strcmp(optarg, "strict") ) {
            mode = kOrdered;
          } else if( !strcmp(optarg, "sync") ) {
            mode = kPreserveSpecial;
          } else {
            usage();
          }
          break;
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

  if( compress_output > 0 && cfg.output_file.size() > 3
      && cfg.output_file.substr(cfg.output_file.size() - 3) != ".gz" )
    cfg.output_file.append(".gz");

  if( debug > 0 ) {
    cout << "input_file        = " << cfg.input_file    << endl;
    cout << "db_file           = " << cfg.db_file       << endl;
    cout << "odef_file         = " << cfg.odef_file     << endl;
    cout << "output_file       = " << cfg.output_file   << endl;
    cout << "compress_output   = " << compress_output   << endl;
    cout << "ordering mode     = " << mode              << endl;
  }
}

//-------------------------------------------------------------
// Wrapper object around event buffer. Includes metadata about the
// buffer contents
class EventBuffer {
public:
  EventBuffer();
  [[nodiscard]] evbuf_t*     get()        const { return m_buffer.get(); }
  [[nodiscard]] evbuf_ptr_t& getptr()           { return m_buffer; }
  [[nodiscard]] size_t       evtnum()     const { return m_evtnum; }
  [[nodiscard]] size_t       size()       const { return m_bufsiz; }
  [[nodiscard]] int          type()       const { return m_type; }
  [[nodiscard]] bool         is_special() const { return m_type != 0; }
  void set( size_t size, size_t evtnum, int type ) {
    m_bufsiz = size; m_evtnum = evtnum; m_type = type;
  }
private:
  evbuf_ptr_t m_buffer;
  size_t m_bufsiz;
  size_t m_evtnum;
  int m_type;
};

EventBuffer::EventBuffer() : m_buffer{}, m_bufsiz(0), m_evtnum(0), m_type(0)
{
  // For simplicity, use a fixed buffer size and the standard allocator
  m_buffer = make_unique<evbuf_t[]>(MAX_EVTSIZE);
}


//-------------------------------------------------------------
class EventReader {
public:
  EventReader( size_t max, const string& filename, unsigned int mark = 100 );
  ~EventReader();
  EventBuffer* operator()();
  [[nodiscard]] EventBuffer* get() const { return m_cur; }
  [[nodiscard]] size_t evtnum() const { return m_count; }
  [[nodiscard]] bool is_special() const;
  void print() const;
  void push( EventBuffer* evt );
private:
  DataFile m_inp;
  size_t m_max;
  size_t m_count;
  size_t m_bufcount;
  unsigned int m_mark;
  EventBuffer* m_cur;  // Current event read
  tbb::concurrent_queue<EventBuffer*> m_queue;

  void print_exit_info( int status ) const;
  void mark_progress() const;
};

EventReader::EventReader( size_t max, const string& filename, unsigned int mark )
  : m_inp(filename)
  , m_max(max)
  , m_count(0)
  , m_bufcount(0)
  , m_mark(mark)
  , m_cur(nullptr)
{
  if( m_inp.Open() != 0 ) {
    ostringstream ostr;
    ostr << "Cannot open input " << filename;
    throw file_io_error(ostr.str());
  }
}

EventBuffer* EventReader::operator()() {
  if( !m_inp.IsOpen() )
    return m_cur = nullptr;

  int st = 0;
  if( m_count < m_max ) {
    if( (st = m_inp.ReadEvent()) == 0 ) {
      size_t evsiz = m_inp.GetEvSize();  // bytes
      int type = 0; //TODO
      ++m_count;
      if( !m_queue.try_pop(m_cur) ) {
        // If we're out of buffers, make a new one. This will quickly settle into a
        // steady state as buffers are returned to the queue by the process() node.
        m_cur = new EventBuffer;
        ++m_bufcount;
      }
      m_cur->set(evsiz, m_count, type);
      std::swap(m_cur->getptr(), m_inp.GetEvBuffer());
      assert(m_cur->size() == evsiz);
      mark_progress();

      return m_cur;
    }
  }
  if( m_mark != 0 && m_count >= m_mark )
    cout << endl;

  print_exit_info(st);
  return m_cur = nullptr;
}

bool EventReader::is_special() const {
  return m_cur->is_special();
}

void EventReader::print() const {
  cout << "Event reader: read/limit: " << m_count << "/" << m_max
       << ", buffers allocated = " << m_bufcount << endl;
}

void EventReader::push( EventBuffer* evt ) {
  m_queue.push(evt);
}

void EventReader::print_exit_info( int st ) const {
  if( debug > 0 ) {
    if( st > 0 )
      cerr << "Reading input ended with error " << st << endl;
    else if( st == -1 )
      cout << "Normal end of file" << endl;
    else if( st == 0 )
      cout << "Event limit reached" << endl;
    else
      assert(false);
    cout << "Read " << m_count << " events" << endl;
  }
}

void EventReader::mark_progress() const {
  if( debug > 1 )
    cout << "Event " << m_count << endl;
  else if( m_mark != 0 ) {
    if( (m_count % m_mark) == 0 ) {
      if( m_count > m_mark )
        cout << "..";
      cout << m_count << flush;
    }
  }
}

EventReader::~EventReader() {
  m_inp.Close();
  while( m_queue.try_pop(m_cur) ) {
    delete m_cur;
    m_cur = nullptr;
  }
}


//-------------------------------------------------------------
class OutputWriter {
public:
  explicit OutputWriter( const string& odat_file );
  Context* operator()( Context* ctxPtr );
  ClockTime_t time() const { return m_time_spent; }
private:
  static void WriteHeader( ostrm_t& os, const Context* ctx );
  static void WriteEvent( ostrm_t& os, const Context* ctx, bool do_header = false );
  struct OutFile {
    OutFile() : m_last_written(0), m_header_written(false) {}
    int open( const string& odat_file ) {
      m_outp.open(odat_file, ios::out | ios::trunc | ios::binary);
      if( !m_outp )
        return 1;
      if( compress_output > 0 )
        m_ostrm.push(gzip_compressor());
      m_ostrm.push(m_outp);
      return 0;
    }
    void close() {
      m_ostrm.reset();
      m_outp.close();
    }
    ofstream m_outp;
    ostrm_t m_ostrm;
    size_t m_last_written;
    bool m_header_written;
  } __attribute__((aligned(128)));

  OutFile m_out_file;
  ClockTime_t m_time_spent;
};

OutputWriter::OutputWriter( const string& odat_file )
  : m_time_spent() {
  // Open output file and set up filter chain
  if( m_out_file.open(odat_file) != 0 ) {
    ostringstream ostr;
    ostr << "Error opening output data file " << odat_file;
    throw file_io_error(ostr.str());
  }
}

Context* OutputWriter::operator()( Context* ctxPtr ) {
  auto start = HighResClock::now();
  ofstream& outp = m_out_file.m_outp;
  ostrm_t& outs = m_out_file.m_ostrm;

  if( !outp.good() || !outs.good() )
    // TODO: handle errors properly
    goto skip;

  if( !m_out_file.m_header_written ) {
    WriteHeader(outs, ctxPtr);
    m_out_file.m_header_written = true;
  }
  WriteEvent(outs, ctxPtr);
skip:
  auto stop = HighResClock::now();
  m_time_spent += stop-start;  // TODO
  return ctxPtr;
}

void OutputWriter::WriteEvent( ostrm_t& os, const Context* const ctx, bool do_header ) {
  // Write output file data (or header names)
  for( const auto& var : ctx->outvars ) {
    var->write(os, do_header);
  }
  if( debug > 1 && !do_header )
    cout << "Wrote nev = " << ctx->nev << endl;
}

void OutputWriter::WriteHeader( ostrm_t& os, const Context* const ctx ) {
  // Write output file header
  // <N = number of variables> N*<variable type> N*<variable name C-string>
  // where
  //  <variable type> = TTTNNNNN,
  // with
  //  TTT   = type (0=int, 1=unsigned, 2=float/double, 3=C-string)
  //  NNNNN = number of bytes
  uint32_t nvars = ctx->outvars.size();
  os.write(reinterpret_cast<const char*>(&nvars), sizeof(nvars));
  for( const auto& var : ctx->outvars ) {
    char type = var->GetType();
    os.write(&type, sizeof(type));
  }
  WriteEvent(os, ctx, true);
}

//-------------------------------------------------------------
class ReadOneEvent {
public:
  explicit ReadOneEvent( EventReader& evread )
  : m_evread(&evread)
  {}
  EventBuffer* operator()(flow_control& fc) {
    auto* ev = (*m_evread)();
    if( !ev ||
        (mode == kPreserveSpecial && ev->is_special()) ) {
      fc.stop();
      return nullptr;
    }
    return ev;
  }
private:
  EventReader* m_evread;
};

using tuple_t = std::tuple<EventBuffer*, Context*>;

//-------------------------------------------------------------
class ProcessEvent {
public:
  explicit ProcessEvent( EventReader& evread )
  : m_evread(&evread)
  {}
  Context* operator()(const tuple_t& t ) {
    auto start = HighResClock::now();

    auto* evtPtr = get<0>(t);
    auto* ctxPtr = get<1>(t);
    auto& ctx = *ctxPtr;
    if( int status = ctx.evdata.Load(evtPtr->get()) ) {
      cerr << "Decoding error = " << status
           << " at event " << ctx.nev << endl;
      goto skip;
    }
    ctx.iseq = ctx.nev = evtPtr->evtnum();
    if( debug > 2 ) {
      cout << "Loaded event " << ctx.nev
           << ", context = " << ctx.id
           << flush << endl;
    }

    for( auto& det: ctx.detectors ) {
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
    (*m_evread).push(evtPtr);

    auto stop = HighResClock::now();
    ctx.m_time_spent += stop - start;

    return ctxPtr;
  }
private:
  EventReader* m_evread;
};

//-------------------------------------------------------------
class OutputEvent {
public:
  explicit OutputEvent( OutputWriter& outw )
    : m_out(&outw)
  {}
  Context* operator()( Context* ctxPtr ) {
    if( debug > 2 ) {
      cout << "Output " << setw(5) << ctxPtr->nev;
      if( ctxPtr->evdata.IsSyncEvent() )
        cout << " S ";
      else
        cout << "   ";
      cout << ", context = " << ctxPtr->id
           << flush << endl;
    }
    auto* ret = (*m_out)(ctxPtr);
    return ret;
  }
private:
  OutputWriter* m_out;
};

//-------------------------------------------------------------
class BenchmarkTimer {
public:
  BenchmarkTimer()
    : start{HighResClock::now()}
    , init_start{HighResClock::now()}
  {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_clock);
  }
  void stop_init() {
    init_duration = HighResClock::now() - init_start;
  }
  void stop( const vector<unique_ptr<Context>>& contexts,
             const OutputWriter& outw ) {
    run_duration = HighResClock::now() - start;
    analysis_realtime_sum =
      std::accumulate(contexts.begin(), contexts.end(), ClockTime_t(),
                      []( const ClockTime_t& val, const auto& ctx ) -> ClockTime_t {
                        return val + ctx->m_time_spent;
                      });
    output_realtime_sum = outw.time();
    // Total CPU time
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_clock);
    timespec_diff(&stop_clock, &start_clock, &clock_diff);
    cpu_usage = std::chrono::seconds(clock_diff.tv_sec)
                + std::chrono::nanoseconds(clock_diff.tv_nsec);

  }
  void print() const {
    cout << "Timing analysis:" << endl;
    cout << "Init:      " << init_duration.count()         << " ms" << endl;
    cout << "Analysis:  " << analysis_realtime_sum.count() << " ms" << endl;
    cout << "Output:    " << output_realtime_sum.count()   << " ms" << endl;
    cout << "Total CPU: " << cpu_usage.count()             << " ms" << endl;
    cout << "Real:      " << run_duration.count()          << " ms" << endl;
  };
private:
  timespec start_clock{}, stop_clock{}, clock_diff{};
  HighResClock::time_point start;
  HighResClock::time_point init_start;
  ClockTime_t init_duration{};
  ClockTime_t run_duration{};
  ClockTime_t analysis_realtime_sum{};
  ClockTime_t output_realtime_sum{};
  ClockTime_t cpu_usage{};
};

//-------------------------------------------------------------
// Read database, if any
static int ReadDatabase()
{
  int sz = database.Open(cfg.db_file);
  if( sz < 0 )
    return sz;
  if( sz > 0 and debug > 0 ) {
    cout << "Read " << sz << " parameters from database " << cfg.db_file << endl;
    if( debug > 1 )
      database.Print();
  }
  return sz;
}

//-------------------------------------------------------------
unsigned int SetNThreads()
{
  unsigned int hwthreads = tbb::global_control::active_value(
    tbb::global_control::max_allowed_parallelism);
  unsigned int nthreads = cfg.nthreads;
  if( nthreads > 2 * hwthreads )
    nthreads = 2 * hwthreads;
  if( nthreads == 0 )
    nthreads = (hwthreads > 1) ? hwthreads : 1;
  tbb::global_control gblctl(tbb::global_control::max_allowed_parallelism, nthreads);
  return nthreads;
}

//-------------------------------------------------------------
int MakeDets( detlst_t& detlst )
{
  detlst.push_back( make_unique<DetectorTypeA>("detA", 1));
  detlst.push_back( make_unique<DetectorTypeB>("detB", 2));
  detlst.push_back( make_unique<DetectorTypeC>("detC", 3));

  // Initialize shared analysis object data
  for( auto& det: detlst ) {
    if( det->Init(true) != 0 )
      // Die on failure to initialize (database read error)
      return 1;
  }
  return 0;
}

//-------------------------------------------------------------
int MakeContexts( vector<unique_ptr<Context>>& contexts,
                  unsigned int nthreads, const detlst_t& detlst )
{
  contexts.clear();
  contexts.reserve(nthreads);
  for( decltype(nthreads) i = 0; i < nthreads; ++i ) {
    // Make new context
    auto ctxPtr = make_unique<Context>(i);
    Context& ctx = *ctxPtr;
    // Clone detectors into each new context
    CopyContainer(detlst, ctx.detectors);
    // Init this context
    assert(!ctx.is_init);
    if( ctx.Init() != 0 )
      // Die on failure to initialize. Should not happen, but be safe.
      return 2;
    contexts.push_back( std::move(ctxPtr) );
  }
  return 0;
}

//-------------------------------------------------------------
// Main program
int main( int argc, char* const* argv )
{
  get_args(argc, argv);

  // Start timers
  BenchmarkTimer timer;

  // Configure max number of threads to use
  auto nthreads = SetNThreads();
  if( debug > 0 )
    cout << "Initializing " << nthreads << " analysis threads" << endl;

  if( ReadDatabase() < 0 )
    return 1;  // error message already printed

  // Set up analysis objects
  detlst_t gDets;
  if( MakeDets(gDets) != 0 )
    return 1;

  // Set up thread contexts. Copy analysis objects.
  vector<unique_ptr<Context>> contexts;
  if( MakeContexts( contexts, nthreads, gDets ) != 0 )
    return 2;
  gDets.clear();  // No need to keep the prototype detector objects around

  // Set up TBB flow graph nodes
  tbb::flow::graph g;
  join_node < tuple_t, reserving > j(g);
  buffer_node<Context*> free_ctx(g);

  // Input
  EventReader eventReader(cfg.nev_max, cfg.input_file);
  input_node<EventBuffer*>
    read_input(g, ReadOneEvent(eventReader));

  // Parallel processing of events in flight
  function_node<tuple_t, Context*>
    process(g, unlimited, ProcessEvent(eventReader));

  // Sequential output
  OutputWriter outputWriter(cfg.output_file);
  function_node<Context*, Context*>
    out(g, serial, OutputEvent(outputWriter));

  // Sequencer for event ordering
  sequencer_node<Context*> seq(g, []( const Context* ctx ) -> size_t {
    return ctx->nev - 1;   // Sequence must start at 0
  });

  // Build the graph
  make_edge(free_ctx, input_port<1>(j));
  make_edge(j, process);
  if( mode == kOrdered ) {
    make_edge(process, seq);
    make_edge(seq, out);
  } else {
    make_edge(process, out);
  }
  make_edge(out, free_ctx);

  for( auto& ctxPtr: contexts ) {
    free_ctx.try_put(ctxPtr.get());
  }

  timer.stop_init();

  if( debug > 0 )
    cout << "Starting event loop, nev_max = " << cfg.nev_max << endl;

  if( mode != kPreserveSpecial ) {
    make_edge(read_input, input_port<0>(j));
    read_input.activate();
    g.wait_for_all();

  } else {
    queue_node<EventBuffer*> evtqueue(g);
    make_edge(evtqueue, input_port<0>(j));
    make_edge(read_input, input_port<0>(j));
    EventBuffer* ev;
    do {
      read_input.activate();
      g.wait_for_all();

      ev = eventReader.get();
      if( ev ) {
        evtqueue.try_put(ev);
        g.wait_for_all();
      }
    } while( ev );
  }

  if( debug > 0 )
    eventReader.print();

  // Total wall times
  timer.stop(contexts, outputWriter);
  timer.print();

  return 0;
}
