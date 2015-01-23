// Prototype parallel processing analyzer

#include "DataFile.h"
#include "Decoder.h"

#include <iostream>
#include <pthread.h>

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

  Decoder evdata;
  
  unsigned long nev = 0;
  while( inp.ReadEvent() == 0 ) {
    int status;
    ++nev;
    // Do some minimal decoding
    if( (status = evdata.Load( inp.GetEvBuffer() )) == 0 ) {
      int ndata = evdata.GetNdata();
      cout << "Event " << nev << ", size = " << evdata.GetEvSize()
	   << ", ndata = " << ndata;
      if( ndata > 0 ) {
	cout << ", data = ";
	for( int i = 0; i < ndata; ++i ) {
	  cout << evdata.GetData(i);
	  if( i+1 != ndata )
	    cout << ", ";
	}
      }
      cout << endl;
    } else {
      cerr << "Decoding error = " << status << " at event " << nev << endl;
      break;
    }
  }
  cout << "Normal end of file" << endl;
  cout << "Read " << nev << " events" << endl;

  return 0;
}
