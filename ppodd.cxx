// Prototype parallel processing analyzer

#include "Podd.h"
#include "DataFile.h"
#include "Decoder.h"
#include "DetectorTypeA.h"
#include "DetectorTypeB.h"
#include "Variable.h"
#include "Output.h"
#include "Util.h"

#include <iostream>
#include <climits>
#include <unistd.h>
#include <pthread.h>

using namespace std;

// Definitions for items declared in Podd.h
detlst_t gDets;
varlst_t gVars;

int debug = 0;
int compress_output = 0;

// Thread processing
struct Context {
  uint32_t* evbuffer;
  Decoder*  evdata;
  detlst_t* detectors;
  int       nev;
};

// Loop:
// - wait for data
// - feed event data to defined analysis object(s)
// - notify someone about end of processing

int AnalyzeEvent( Context& ctx )
{
  // Process all defined analysis objects

  int status = ctx.evdata->Load( ctx.evbuffer );
  if( status != 0 ) {
    cerr << "Decoding error = " << status
	 << " at event " << ctx.nev << endl;
    return 2;
  }
  for( detlst_t::iterator it = (*ctx.detectors).begin();
       it != (*ctx.detectors).end(); ++it ) {
    int status;
    Detector* det = *it;

    det->Clear();
    if( (status = det->Decode(*ctx.evdata)) != 0 )
      return status;
    if( (status = det->Analyze()) != 0 )
      return status;
  }
  return 0;
}

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
  int nthreads = GetCPUcount();

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

  if( debug > 0 )
    PrintVarList(gVars);

  // Copy analysis object into thread contexts
  Context ctx;
  ctx.detectors = &gDets;

  // Start threads

  // Open input
  DataFile inp(input_file.c_str());
  if( inp.Open() )
    return 2;

  // Configure output
  Output output;
  if( compress_output > 0 && odat_file.size() > 3
      && odat_file.substr(odat_file.size()-3) != ".gz" )
    odat_file.append(".gz");
  if( output.Init(odat_file.c_str(), odef_file.c_str(), gVars) != 0 ) {
    inp.Close();
    return 3;
  }

  Decoder evdata;
  ctx.evdata = &evdata;

  unsigned long nev = 0;

  // Loop: Read one event and hand it off to an idle thread
  while( inp.ReadEvent() == 0 && nev < nev_max ) {
    int status;
    ++nev;
    if( mark && (nev%1000) == 0 )
      cout << nev << endl;



      // Main processing
      if( debug > 1 )
	cout << "Event " << nev << endl;

      ctx.evbuffer = inp.GetEvBuffer();
      if( (status = AnalyzeEvent(ctx)) != 0 ) {
	cerr << "Analysis error = " << status << " at event " << nev << endl;
	break;
      }

      if( debug > 1 )
	PrintVarList(gVars);

      // Write output
      if( (status = output.Process(nev)) != 0 ) {
	cerr << "Output error = " << status << endl;
	break;
      }
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
