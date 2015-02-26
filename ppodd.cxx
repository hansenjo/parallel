// Prototype parallel processing analyzer

#include "Podd.h"
#include "DataFile.h"
#include "Decoder.h"
#include "DetectorTypeA.h"
#include "DetectorTypeB.h"
#include "Variable.h"
#include "Output.h"
#include "Util.h"
#include "ThreadPool.hpp"

#include <iostream>
#include <climits>
#include <unistd.h>

// For output module
#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;
using namespace ThreadUtil;
using namespace boost::iostreams;

// Definitions of items declared in Podd.h
detlst_t gDets;
varlst_t gVars;

int debug = 0;
int compress_output = 0;

// Thread context
struct Context {
  Context();
  // Per-thread data
  vector<evbuf_t> evbuffer; // Copy of event buffer read from file
  Decoder   evdata;      // Decoded data
  detlst_t  detectors;   // Detectors with private event-by-event data
  varlst_t  variables;   // Interface to analysis resuls
  Output    output; //TODO: not sure how to handle this yet
  int       nev;         // Event number given to this thread
  int       id;          // This thread's ID

  static const int INIT_EVSIZE = 1024;
};

Context::Context()
{
  evbuffer.reserve(INIT_EVSIZE);
}

template <typename Context>
class AnalysisThread : public PoolWorkerThread<Context>
{
public:
  AnalysisThread( WorkQueue<Context>& wq, WorkQueue<Context>& fq,
		  WorkQueue<Context>& rq )
    : PoolWorkerThread<Context>(wq,fq,rq)
  {
  }

protected:
  virtual void run()
  {
    while( Context* ctx = this->fWorkQueue.next() ) {

      // Process all defined analysis objects

      int status = ctx->evdata.Load( &ctx->evbuffer[0] );
      if( status != 0 ) {
	cerr << "Decoding error = " << status
	     << " at event " << ctx->nev << endl;
	return; //2
      }
      for( detlst_t::iterator it = ctx->detectors.begin();
	   it != ctx->detectors.end(); ++it ) {
	int status;
	Detector* det = *it;

	det->Clear();
	if( (status = det->Decode(ctx->evdata)) != 0 )
	  return;// status;
	if( (status = det->Analyze()) != 0 )
	  return;// status;
      }
      this->fResultQueue.add(ctx);
    }
  }
private:
};

template <typename Pool_t, typename Context>
class OutputThread : public Thread
{
public:
  OutputThread( Pool_t& pool, const char* odat_file ) : fPool(pool)
  {
    if( !odat_file ||!*odat_file )
      return; // TODO: throw exception

    // Open output file and set up filter chain
    outs.reset();
    outp.close();
    outp.open( odat_file, ios::out|ios::trunc|ios::binary);
    if( !outp ) {
      cerr << "Error opening output data file " << odat_file << endl;
      return;
    }
    if( compress_output > 0 )
      outs.push(gzip_compressor());
    outs.push(outp);

    outs << "Header goes here ... ";
    // outs << "Event" << field_sep;
    // for( vvec_t::iterator it = vars.begin(); it != vars.end(); ) {
    //   Variable* var = *it;
    //   outs << var->GetName();
    //   ++it;
    //   if( it != vars.end() )
    // 	outs << field_sep;
    // }
    outs << endl;
  }

  ~OutputThread()
  {
    Close();
  }

  int Close()
  {
    outs.reset();
    outp.close();
    return 0;
  }

protected:
  virtual void run()
  {
    while( Context* ctx = fPool.nextResult() ) {
      ctx->output.Process( ctx->nev );
      // TODO: handle errors
      fPool.addFreeData(ctx);
    }
  }
private:
  Pool_t& fPool;
  std::ofstream outp;
  boost::iostreams::filtering_ostream outs;
};

static string prgname;

static void default_names( string infile, string& odef, string& odat )
{
  // If not given, set defaults for odef and output files
  if( infile.empty() )
    return;
  string::size_type pos = infile.rfind('.');
  if( pos != string::npos )
    infile.erase(pos);
  // Ignore any directory component in the file name
  pos = infile.rfind('/');
  if( pos != string::npos && pos+1 < infile.size() )
    infile.erase(0,pos+1);
  if( odef.empty() )
    odef = infile + ".odef";
  if( odat.empty() )
    odat = infile + ".odat";
}

static void usage()
{
  cerr << "Usage: " << prgname << " [options] input_file.dat" << endl
       << "where options are:" << endl
       << " [ -c odef_file ]\tread output definitions from odef_file"
       << " (default = input_file.odef)" << endl
       << " [ -o outfile ]\t\twrite output to odat_file"
       << " (default = input_file.odat)" << endl
       << " [ -d debug_level ]\tset debug level" << endl
       << " [ -n nev_max ]\t\tset max number of events" << endl
       << " [ -t nthreads ]\t\tcreate at most nthreads (default = n_cpus)" << endl
       << " [ -m ]\t\t\tMark progress" << endl
       << " [ -z ]\t\t\tCompress output with gzip" << endl
       << " [ -h ]\t\t\tPrint this help message" << endl;
  exit(255);
}

int main( int argc, char* const *argv )
{
  // Parse command line
  unsigned long nev_max = ULONG_MAX;
  int opt;
  bool mark = false;
  string input_file, odef_file, odat_file;
  int ncpu = GetCPUcount();
  int nthreads = ncpu-1;

  prgname = argv[0];
  if( prgname.size() >= 2 && prgname.substr(0,2) == "./" )
    prgname.erase(0,2);

  while( (opt = getopt(argc, argv, "c:d:n:o:t:zmh")) != -1 ) {
    switch (opt) {
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
    case 't':
      nthreads = atoi(optarg);
      break;
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

  default_names( input_file, odef_file, odat_file );

  // Set up analysis objects
  gDets.push_back( new DetectorTypeA("detA",1) );
  gDets.push_back( new DetectorTypeB("detB",2) );

  // Initialize
  // (This could be one thread per detector)
  int err = 0;
  for( detlst_t::iterator it = gDets.begin(); it != gDets.end(); ++it ) {
    int status;
    Detector* det = *it;
    if( (status = det->Init()) != 0 ) {
      err = status;
      cerr << "Error initializing detector " << det->GetName() << endl;
    }
  }
  if( err )
    return 1;

  if( debug > 1 )
    PrintVarList(gVars);

  // Set up thread contexts. Copy analysis objects.
  if( nthreads <= 0 || nthreads >= ncpu )
    nthreads = (ncpu > 1) ? ncpu-1 : 1;
  if( debug > 0 )
    cout << "Initializing " << nthreads << " analysis threads" << endl;

  // Start threads

  // Open input
  DataFile inp(input_file.c_str());
  if( inp.Open() )
    return 2;

  typedef ThreadPool<AnalysisThread,Context> thread_pool_t;

  thread_pool_t pool(nthreads);

  // Configure output
  if( compress_output > 0 && odat_file.size() > 3
      && odat_file.substr(odat_file.size()-3) != ".gz" )
    odat_file.append(".gz");
  //Output output;
  // if( output.Init( odat_file.c_str(), odef_file.c_str(),
  // 		   ctx[0].variables) != 0 ) {
  //   inp.Close();
  //   return 3;
  // }
  OutputThread<thread_pool_t,Context> output( pool, odat_file.c_str() );
  output.start();

  unsigned long nev = 0;

  vector<Context> ctx(nthreads);
  for( vector<Context>::size_type i = 0; i < ctx.size(); ++i ) {
    // shallow copy for now, to be changed
    ctx[i].detectors = gDets;
    ctx[i].variables = gVars;
    pool.addFreeData( &ctx[i] );
  }

  // Loop: Read one event and hand it off to an idle thread
  while( inp.ReadEvent() == 0 && nev < nev_max ) {
    //    int status;
    ++nev;
    if( mark && (nev%1000) == 0 )
      cout << nev << endl;
    // Main processing
    if( debug > 1 )
      cout << "Event " << nev << endl;

    Context* curCtx = pool.nextFree();

    curCtx->evbuffer.assign( inp.GetEvBuffer(),
			     inp.GetEvBuffer()+inp.GetEvWords() );
    curCtx->nev = nev;
    pool.Process( curCtx );
  }
  if( debug > 0 ) {
    cout << "Normal end of file" << endl;
    cout << "Read " << nev << " events" << endl;
  }

  inp.Close();
  output.Close();

  DeleteContainer( gDets );

  return 0;
}
