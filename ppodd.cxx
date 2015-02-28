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
#include <algorithm>  // for std::swap

// For output module
#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;
using namespace ThreadUtil;
using namespace boost::iostreams;

// Definitions of items declared in Podd.h
detlst_t gDets;

int debug = 0;
int compress_output = 0;
int delay_us = 0;

typedef vector<OutputElement*> voutp_t;

// Thread context
struct Context {
  Context();
  ~Context();
  int Init( const char* odef_file );

  // Per-thread data
  evbuf_t*  evbuffer;    // Event buffer read from file
  Decoder   evdata;      // Decoded data
  detlst_t  detectors;   // Detectors with private event-by-event data
  varlst_t  variables;   // Interface to analysis results
  voutp_t   outvars;     // Output definitions
  int       nev;         // Event number given to this thread
  int       id;          // This thread's ID
  bool      is_init;     // Init() called successfully

  static const int INIT_EVSIZE = 1024;
};

Context::Context() : evbuffer(0), is_init(false)
{
}

Context::~Context()
{
  DeleteContainer( outvars );
  DeleteContainer( detectors );
  DeleteContainer( variables );
  delete [] evbuffer;
}

int Context::Init( const char* odef_file )
{
  // Initialize current context

  // Initialize detectors
  int err = 0;
  for( detlst_t::iterator it = detectors.begin(); it != detectors.end(); ++it ) {
    int status;
    Detector* det = *it;
    det->SetVarList(variables);
    if( (status = det->Init()) != 0 ) {
      err = status;
      cerr << "Error initializing detector " << det->GetName() << endl;
    }
  }
  if( err )
    return 1;

  // Read output definitions & configure output
  if( !odef_file || !*odef_file )
    return 2;

  outvars.clear();

  ifstream inp(odef_file);
  if( !inp ) {
    cerr << "Error opening output definition file " << odef_file << endl;
    return 2;
  }
  string line;
  while( getline(inp,line) ) {
    // Wildcard match
    string::size_type pos = line.find_first_not_of(" \t");
    if( line.empty() || pos == string::npos || line[pos] == '#' )
      continue;
    line.erase(0,pos);
    pos = line.find_first_of(" \t");
    if( pos != string::npos )
      line.erase(pos);
    for( varlst_t::const_iterator it = variables.begin();
	 it != variables.end(); ++it ) {
      Variable* var = *it;
      if( WildcardMatch(var->GetName(), line) )
	outvars.push_back( new PlainVariable(var) );
    }
  }
  inp.close();

  if( outvars.empty() ) {
    // Noting to do
    cerr << "No output variables defined. Check " << odef_file << endl;
    return 3;
  }

  evbuffer = new evbuf_t[INIT_EVSIZE];
  evbuffer[0] = 0;

  is_init = true;
  return 0;
}

template <typename Context_t>
class AnalysisThread : public PoolWorkerThread<Context_t>
{
public:
  AnalysisThread( WorkQueue<Context_t>& wq, WorkQueue<Context_t>& fq,
		  WorkQueue<Context_t>& rq )
    : PoolWorkerThread<Context_t>(wq,fq,rq), fSeed(pthread_self())
  {
    srand(fSeed);
  }

protected:
  virtual void run()
  {
    while( Context_t* ctx = this->fWorkQueue.next() ) {

      // Process all defined analysis objects

      int status = ctx->evdata.Load( ctx->evbuffer );
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

      // If requested, add random delay
      if( delay_us > 0 )
	usleep((unsigned int)((float)rand_r(&fSeed)/(float)RAND_MAX*2*delay_us));

      this->fResultQueue.add(ctx);
    }
  }
private:
  unsigned int fSeed;
};

static const char* field_sep = ", ";

template <typename Pool_t, typename Context_t>
class OutputThread : public Thread
{
public:
  OutputThread( Pool_t& pool, const char* odat_file )
    : fPool(pool), fHeaderWritten(false)
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
    if( !outp.is_open() )
      return;

    while( Context_t* ctx = fPool.nextResult() ) {

      if( !outp.good() || !outs.good() )
	// TODO: handle errors properly
	return;

      if( !fHeaderWritten ) {
	outs << "Event";
	if( !ctx->outvars.empty() )
	  outs << field_sep;
	for( voutp_t::const_iterator it = ctx->outvars.begin();
	     it != ctx->outvars.end(); ) {
	  OutputElement* var = *it;
	  outs << var->GetName();
	  ++it;
	  if( it != ctx->outvars.end() )
	    outs << field_sep;
	}
	outs << endl;
	fHeaderWritten = true;
      }

      outs << ctx->nev;
      if( !ctx->outvars.empty() )
	outs << field_sep;
      for( voutp_t::const_iterator it = ctx->outvars.begin();
	   it != ctx->outvars.end(); ) {
	OutputElement* var = *it;
	var->write( outs );
	++it;
	if( it != ctx->outvars.end() )
	  outs << field_sep;
      }
      outs << endl;

      fPool.addFreeData(ctx);
    }
  }
private:
  Pool_t& fPool;
  std::ofstream outp;
  boost::iostreams::filtering_ostream outs;
  bool fHeaderWritten;
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
       << " [ -y us ]\t\t\tAdd us microseconds average random delay per event" << endl
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

  while( (opt = getopt(argc, argv, "c:d:n:o:t:y:zmh")) != -1 ) {
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
    case 'y':
      delay_us = atoi(optarg);
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

  // if( debug > 1 )
  //   PrintVarList(gVars);

  // Set up thread contexts. Copy analysis objects.
  if( nthreads <= 0 || nthreads >= ncpu )
    nthreads = (ncpu > 1) ? ncpu-1 : 1;
  if( debug > 0 )
    cout << "Initializing " << nthreads << " analysis threads" << endl;

  typedef ThreadPool<AnalysisThread,Context> thread_pool_t;

  thread_pool_t pool(nthreads);

  vector<Context> ctx(nthreads);
  for( vector<Context>::size_type i = 0; i < ctx.size(); ++i ) {
    // Deep copy of container objects
    CopyContainer( gDets, ctx[i].detectors );
    pool.addFreeData( &ctx[i] );
  }

  // Configure output
  if( compress_output > 0 && odat_file.size() > 3
      && odat_file.substr(odat_file.size()-3) != ".gz" )
    odat_file.append(".gz");

  OutputThread<thread_pool_t,Context> output( pool, odat_file.c_str() );
  output.start();

  unsigned long nev = 0;

  // Open input
  DataFile inp(input_file.c_str());
  if( inp.Open() )
    return 2;

  // Loop: Read one event and hand it off to an idle thread
  while( inp.ReadEvent() == 0 && nev < nev_max ) {
    ++nev;
    if( mark && (nev%1000) == 0 )
      cout << nev << endl;
    // Main processing
    if( debug > 1 )
      cout << "Event " << nev << endl;

    Context* curCtx = pool.nextFree();
    // Init if necessary
    //TODO: split up Init:
    // (1) Read database and all other related things, do before cloning
    //     detectors
    // (2) DefineVariables: do in threads
    if( !curCtx->is_init ) {
      if( curCtx->Init(odef_file.c_str()) != 0 )
	break;
    };

    swap( curCtx->evbuffer, inp.GetEvBuffer() );
    curCtx->nev = nev;
    pool.Process( curCtx );
  }
  if( debug > 0 ) {
    cout << "Normal end of file" << endl;
    cout << "Read " << nev << " events" << endl;
  }

  inp.Close();

  // Terminate worker threads
  pool.finish();
  pool.addResult(0);
  output.join();
  output.Close();

  DeleteContainer( gDets );

  return 0;
}
