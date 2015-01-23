// Prototype parallel processing analyzer

#include <iostream>
#include <pthread.h>
#include "DataFile.h"
#include <cstring>

using namespace std;

// Thread processing
// Loop:
// - wait for data
// - feed event data to defined analysis object(s)
// - notify someone about end of processing


int main( int argc, const char** argv )
{
  // Parse command line

  // Set up analysis object

  // Copy analysis object into thread contexts 

  // Start threads

  // Loop: Read one event and hand it off to an idle thread


  DataFile inp("test.dat");
  if( inp.Open() )
    return 1;

  unsigned long nev = 0;
  while( inp.ReadEvent() == 0 ) {
    ++nev;
    // Do some minimal decoding
    uint32_t* evbuffer = inp.GetEvBuffer();
    size_t evsize = evbuffer[0];
    if( evsize < 8 ) {
      cerr << "Event " << nev << ", bad event size = "
	   << evsize << endl;
      return 2;
    }
    uint32_t ndata = evbuffer[1];
    cout << "Event " << nev << ", size = " << evsize << ", ndata = " << ndata;
    if( ndata > 0 ) {
      double x;
      cout << ", data = ";
      for( uint32_t i = 0; i < ndata; ++i ) {
	const size_t stepsize = sizeof(x)/sizeof(evbuffer[0]);
	memcpy( &x, evbuffer+2+stepsize*i, sizeof(double) );
	cout << x;
	if( i+1 != ndata )
	  cout << ", ";
      }
    }
    cout << endl;
  }
  cout << "Normal end of file" << endl;
  cout << "Read " << nev << " events" << endl;

  return 0;
}
