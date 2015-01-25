// Prototype parallel processing analyzer

#include "Podd.h"
#include "DataFile.h"
#include "Decoder.h"
#include "DetectorTypeA.h"
#include "Variable.h"
#include "Util.h"

#include <iostream>
#include <pthread.h>

using namespace std;

// Definitions for items declared in Podd.h
detlst_t gDets;
varlst_t gVars;

int debug = 3;

// Thread processing
struct Context {
  Decoder*  evdata;
  detlst_t* detectors;
};

// Loop:
// - wait for data
// - feed event data to defined analysis object(s)
// - notify someone about end of processing

int AnalyzeEvent( Context& ctx )
{
  // Process all defined analysis objects

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

int main( int argc, const char** argv )
{
  // Parse command line

  // Set up analysis objects
  gDets.push_back( new DetectorTypeA("detA") );

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
  DataFile inp("test.dat");
  if( inp.Open() )
    return 2;

  Decoder evdata;

  unsigned long nev = 0;

  // Loop: Read one event and hand it off to an idle thread
  while( inp.ReadEvent() == 0 ) {
    int status;
    ++nev;
    if( (status = evdata.Load( inp.GetEvBuffer() )) == 0 ) {
      // Main processing
      if( debug > 1 )
	cout << "Event " << nev << ", size = " << evdata.GetEvSize() << endl;
      ctx.evdata = &evdata;
      if( (status = AnalyzeEvent(ctx)) != 0 ) {
	cerr << "Detector error = " << status << " at event " << nev << endl;
	break;
      }
      if( debug > 1 )
	PrintVarList(gVars);
    } else {
      cerr << "Decoding error = " << status << " at event " << nev << endl;
      break;
    }
  }
  cout << "Normal end of file" << endl;
  cout << "Read " << nev << " events" << endl;

  inp.Close();

  DeleteContainer( gDets );

  return 0;
}
