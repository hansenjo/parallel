// Prototype parallel processing analyzer

#include "DataFile.h"
#include "Decoder.h"
#include "Detector.h"

#include <iostream>
#include <pthread.h>
#include <list>

using namespace std;

typedef list<Detector*> detlst_t;

struct Context {
  Decoder*  evdata;
  detlst_t* detectors;
};

// Thread processing
// Loop:
// - wait for data
// - feed event data to defined analysis object(s)
// - notify someone about end of processing

int DoAnalysis( Context& ctx )
{
  // Process all defined analysis objects

  for( detlst_t::iterator it = (*ctx.detectors).begin();
       it != (*ctx.detectors).end(); ++it ) {
    int status;
    Detector* det = *it;

    det->Clear();
    if( (status = det->Decode(*ctx.evdata)) != 0 )
      return status;
    if( (status = det->Analyze(*ctx.evdata)) != 0 )
      return status;
  }

  return 0;
}

int main( int argc, const char** argv )
{
  // Parse command line

  // Set up analysis object

  // Copy analysis object into thread contexts

  // Start threads

  // Loop: Read one event and hand it off to an idle thread

  detlst_t gDets;
  gDets.push_back( new Detector("det1") );

  Context ctx;
  ctx.detectors = &gDets;

  DataFile inp("test.dat");
  if( inp.Open() )
    return 1;

  Decoder evdata;
  
  unsigned long nev = 0;
  while( inp.ReadEvent() == 0 ) {
    int status;
    ++nev;
    // Do some minimal decoding
    if( (status = evdata.Load( inp.GetEvBuffer() )) == 0 ) {
      cout << "Event " << nev << ", size = " << evdata.GetEvSize() << endl;
      ctx.evdata = &evdata;
      if( (status = DoAnalysis(ctx)) != 0 ) {
	cerr << "Detector error = " << status << " at event " << nev << endl;
	break;
      }
    } else {
      cerr << "Decoding error = " << status << " at event " << nev << endl;
      break;
    }
  }
  cout << "Normal end of file" << endl;
  cout << "Read " << nev << " events" << endl;

  inp.Close();

  return 0;
}
