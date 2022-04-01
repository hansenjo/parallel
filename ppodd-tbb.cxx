// Prototype parallel processing analyzer

#include "Podd.h"
#include "DataFile.h"
//#include "Decoder.h"
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
#include <oneapi/tbb/scalable_allocator.h>
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
scalable_allocator<evbuf_t> event_allocator;

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
}

//-------------------------------------------------------------
static void mark_progress( size_t nev )
{
  if( cfg.mark != 0 ) {
    if( (nev % cfg.mark) == 0 ) {
      if( nev > cfg.mark )
        cout << "..";
      cout << nev << flush;
    }
  }
}

//-------------------------------------------------------------
class EventReader {
public:
  explicit EventReader( size_t max, const string& filename );
  ~EventReader();
  evbuf_t* operator()();
  [[nodiscard]] evbuf_t* get() const { return m_cur; }
  [[nodiscard]] size_t evtnum() const { return m_count; }
  [[nodiscard]] bool is_special() const;
  void print() const;
  void push( evbuf_t* evt );
private:
  size_t m_max;
  size_t m_count;
  size_t m_bufcount;
  evbuf_t* m_cur;  // Current event read
  DataFile m_inp;
  tbb::concurrent_queue<evbuf_t*> m_queue;
};

EventReader::EventReader( size_t max, const string& filename ) :
  m_max(max),
  m_count(0),
  m_bufcount(0),
  m_cur(nullptr),
  m_inp(filename) {}

evbuf_t* EventReader::operator()() {
  if( !m_inp.IsOpen() && m_inp.Open() != 0 )
    return m_cur = nullptr;

  int st = m_inp.ReadEvent();
  if( st == 0 && ++m_count <= m_max ) {
    size_t evsiz = m_inp.GetEvSize();  // bytes
    size_t type = 0; //TODO
    constexpr size_t hdrsiz = 3 * sizeof(size_t) / sizeof(evbuf_t); // Space for event header
  get_buffer:
    if( m_queue.try_pop(m_cur) ) {
      auto* hdr = reinterpret_cast<size_t*>(m_cur);
      hdr[0] = evsiz + hdrsiz;
      hdr[1] = m_count;
      hdr[2] = type;
      memcpy(m_cur + hdrsiz, m_inp.GetEvBufPtr(), evsiz); //TODO: swap

      if( debug > 1 )
        cout << "Event " << m_count << endl;
      else
        mark_progress(m_count);

      return m_cur;
    }
    // If we're out of buffers, make a new one. This will quickly settle into a
    // steady state as buffers are returned to the queue by the process() node.
    push(event_allocator.allocate(evsiz + hdrsiz));
    ++m_bufcount;
    goto get_buffer;

  }
  if( cfg.mark != 0 && m_count >= cfg.mark )
    cout << endl;
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
  return m_cur = nullptr;
}

bool EventReader::is_special() const {
  // simulate special event type
  return (((size_t*)m_cur)[2] != 0);
}

void EventReader::print() const {
  cout << "Event reader: read/limit: " << m_count << "/" << m_max
       << ", buffers allocated = " << m_bufcount << endl;
}

void EventReader::push( evbuf_t* evt ) {
  m_queue.push(evt);
}

EventReader::~EventReader() {
  m_inp.Close();
  while( m_queue.try_pop(m_cur) && m_cur )
    event_allocator.deallocate(m_cur, ((size_t*) m_cur)[0]);
}


//-------------------------------------------------------------
class OutputWriter {
public:
  explicit OutputWriter( const string& odat_file );
  Context* operator()( Context* ctxPtr );
  ClockTime_t time() const { return m_time_spent; }
private:
  void WriteHeader( ostrm_t& os, Context* ctx );
  void WriteEvent( ostrm_t& os, Context* ctx, bool do_header = false );
  struct OutFile {
    OutFile() : fLastWritten(0), fHeaderWritten(false) {}
    ~OutFile() { close(); }
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
    ofstream outp;
    ostrm_t outs;
    size_t fLastWritten;
    bool fHeaderWritten;
  } __attribute__((aligned(128)));

  OutFile fOutFile;
  ClockTime_t m_time_spent;
};

OutputWriter::OutputWriter( const string& odat_file )
  : m_time_spent() {
  // Open output file and set up filter chain
  if( fOutFile.open(odat_file) != 0 ) {
    cerr << "Error opening output data file " << odat_file << endl;
    return; // TODO: throw exception
  }
}

Context* OutputWriter::operator()( Context* ctxPtr ) {
  auto start = HighResClock::now();
  ofstream& outp = fOutFile.outp;
  ostrm_t& outs = fOutFile.outs;

  if( !outp.good() || !outs.good() )
    // TODO: handle errors properly
    goto skip;

  if( !fOutFile.fHeaderWritten ) {
    WriteHeader(outs, ctxPtr);
    fOutFile.fHeaderWritten = true;
  }
  WriteEvent(outs, ctxPtr);
skip:
  auto stop = HighResClock::now();
  m_time_spent += stop-start;  // TODO
  return ctxPtr;
}

void OutputWriter::WriteEvent( ostrm_t& os, Context* ctx, bool do_header ) {
  // Write output file data (or header names)
  for( auto& var : ctx->outvars ) {
    var->write(os, do_header);
  }
  if( debug > 1 && !do_header )
    cout << "Wrote nev = " << ctx->nev << endl;
}

void OutputWriter::WriteHeader( ostrm_t& os, Context* ctx ) {
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

//-------------------------------------------------------------
// Main program
int main( int argc, char* const* argv )
{
  get_args(argc, argv);

  if( debug > 0 ) {
    cout << "input_file        = " << cfg.input_file    << endl;
    cout << "db_file           = " << cfg.db_file       << endl;
    cout << "odef_file         = " << cfg.odef_file     << endl;
    cout << "output_file       = " << cfg.output_file   << endl;
    cout << "compress_output   = " << compress_output   << endl;
    cout << "ordering mode     = " << mode              << endl;
  }

  detlst_t gDets;
  vector<unique_ptr<Context>> contexts;

  // Start timers
  timespec start_clock{}, stop_clock{}, clock_diff{};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_clock);
  auto start = HighResClock::now();
  auto init_start = HighResClock::now();

  // Read database, if any
  database.Open(cfg.db_file);
  if( auto sz = database.GetSize(); debug > 0 and sz > 0 ) {
    cout << "Read " << sz << " parameters from database " << cfg.db_file << endl;
    if( debug > 1 )
      database.Print();
  }

  // Configure max number of threads to use
  unsigned int hwthreads = tbb::global_control::active_value(
    tbb::global_control::max_allowed_parallelism), nthreads = cfg.nthreads;
  if( nthreads > 2 * hwthreads )
    nthreads = 2 * hwthreads;
  if( nthreads == 0 )
    nthreads = (hwthreads > 1) ? hwthreads : 1;
  if( debug > 0 )
    cout << "Initializing " << nthreads << " analysis threads" << endl;
  tbb::global_control gblctl(tbb::global_control::max_allowed_parallelism, nthreads);

  // Set up analysis objects
  gDets.push_back( make_unique<DetectorTypeA>("detA", 1));
  gDets.push_back( make_unique<DetectorTypeB>("detB", 2));
  gDets.push_back( make_unique<DetectorTypeC>("detC", 3));

  // Set up thread contexts. Copy analysis objects.
  for( unsigned int i = 0; i<nthreads; ++i ) {
    // Make new context
    auto ctxPtr = make_unique<Context>(i);
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
    contexts.push_back( std::move(ctxPtr) );
  }
  gDets.clear();  // No need to keep the prototype detector objects around

  // Prepare input
  EventReader eventReader(cfg.nev_max, cfg.input_file);

  // Configure output
  if( compress_output > 0 && cfg.output_file.size() > 3
      && cfg.output_file.substr(cfg.output_file.size() - 3) != ".gz" )
    cfg.output_file.append(".gz");

  OutputWriter outputWriter(cfg.output_file);

  // Set up TBB flow graph nodes
  using tuple_t = tuple<evbuf_t*, Context*>;

  tbb::flow::graph g;

  join_node < tuple_t, reserving > j(g);

  buffer_node<Context*> free_ctx(g);

  input_node<evbuf_t*> read_input(g,
                                  [&]( flow_control& fc ) -> evbuf_t* {
                                    auto* ev = eventReader();
                                    if( !ev ||
                                        (mode == kPreserveSpecial && eventReader.is_special() )) {
                                      fc.stop();
                                      return nullptr;
                                    }
                                    return ev;
                                  });

  function_node<tuple_t, Context*> process(g, unlimited,
                                            [&]( const tuple_t& t ) -> Context* {
                                              auto start = HighResClock::now();

                                              auto* evtPtr = get<0>(t);
                                              auto* ctxPtr = get<1>(t);
                                              auto& ctx = *ctxPtr;
                                              if( int status = ctx.evdata.Load(evtPtr+6) ) {
                                                cerr << "Decoding error = " << status
                                                     << " at event " << ctx.nev << endl;
                                                goto skip;
                                              }
//                                              ctx.iseq = ctx.nev = ((size_t*)evtPtr)[1];
//                                              cout << "Loaded event " << ctx.nev
//                                                   << ", context = " << ctx.id
//                                                   << flush << endl;

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
                                              eventReader.push(evtPtr);

                                              auto stop = HighResClock::now();
                                              ctx.m_time_spent += stop-start;

                                              return ctxPtr;
                                            });

  function_node<Context*, Context*> out(g, serial,
                                            [&]( Context* ctxPtr ) -> Context* {
//                                              cout << "Output " << setw(5) << ctxPtr->nev;
////                                              if( ctxPtr->evtype() )
////                                                cout << " S ";
////                                              else
//                                              cout << "   ";
//                                              cout << ", context = " << ctxPtr->id
//                                                   << flush << endl;
                                              auto* ret = outputWriter(ctxPtr);
                                              return ret;
                                            });

  sequencer_node<Context*> seq(g,
                                 []( const Context* ctx ) -> size_t {
                                   return ctx->nev - 1;  // Sequence must start at 0
                                 });

  read_input.activate();

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

  ClockTime_t init_duration = HighResClock::now() - init_start;

  if( debug > 0 )
    cout << "Starting event loop, nev_max = " << cfg.nev_max << endl;

  if( mode != kPreserveSpecial ) {
    make_edge(read_input, input_port<0>(j));
    g.wait_for_all();

  } else {
    queue_node<evbuf_t*> evtqueue(g);
    make_edge(evtqueue, input_port<0>(j));
    make_edge(read_input, input_port<0>(j));
    evbuf_t* ev;
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

  // Total wall times
  ClockTime_t run_duration = HighResClock::now() - start;
  ClockTime_t analysis_realtime_sum =
    std::accumulate(contexts.begin(), contexts.end(), ClockTime_t(),
                    []( const ClockTime_t& val, const auto& ctx ) -> ClockTime_t {
                      return val + ctx->m_time_spent;
                    });
  ClockTime_t output_realtime_sum = outputWriter.time();
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
