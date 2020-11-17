// Generate and write test data file
//
// For the file format generated, see rawdata.h

#include "rawdata.h"

#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <cassert>

using namespace std;

void gauss( double& y1, double& y2 )
{
  // Generate a pair of Gaussian-distributed random numbers.
  // Results are in y1 and y2.
  double x1, x2, w;

  do {
    x1 = 2.0 * drand48() - 1.0;
    x2 = 2.0 * drand48() - 1.0;
    w = x1 * x1 + x2 * x2;
  } while ( w >= 1.0 );

  w = sqrt( (-2.0 * log( w ) ) / w );
  y1 = x1 * w;
  y2 = x2 * w;
}

static const char* prgname;

static void usage()
{
  cerr << "Usage: " << prgname << " [options] output_file" << endl
       << "where options are:" << endl
       << " [ -c num ]\tnumber of detectors to simulate (default 1)" << endl
       << " [ -n nev_max ]\t\tset number of events (default 10000)" << endl
       << " [ -d debug_level ]\tset debug level (default 0)" << endl
       << " [ -h ]\t\t\tprint this help message" << endl;
  exit(255);
}

int main( int argc, char** argv )
{
  const char* filename = "";
  const size_t SIZE = 1024;
  const long int seed = 87934;
  int debug = 0;
  int NEVT = 10000;
  int NDET = 1;
  int opt;

  // Parse command line

  prgname = argv[0];
  if( strlen(prgname) >= 2 && strncmp(prgname,"./",2) == 0 )
    prgname += 2;

  while( (opt = getopt(argc, argv, "c:d:n:h")) != -1 ) {
    switch (opt) {
    case 'c':
      NDET = atoi(optarg);
      if( NDET > MAXMODULES ) {
	cerr << "Too many detectors, max " << MAXMODULES << endl;
	exit(255);
      }
      break;
    case 'd':
      debug = atoi(optarg);
      break;
    case 'n':
      NEVT = atoi(optarg);
      break;
    case 'h':
    default:
      usage();
      break;
    }
  }
  if( optind >= argc ) {
    cerr << "Output file name missing" << endl;
    usage();
  }
  filename = argv[optind];

  // Open output
  FILE* file = fopen(filename, "wb");
  if( !file ) {
    cerr << "Cannot open file " << filename << endl;
    exit(1);
  }

  srand48(seed);
  uint32_t evbuffer[SIZE];

  // Generate event data
  for( int iev = 0; iev < NEVT; ++iev ) {
    EventHeader evthdr;
    evthdr.event_info = NDET;
    char* evtp = (char*)evbuffer;
    memcpy( evtp, &evthdr, sizeof(evthdr) );
    evtp += sizeof(evthdr);
    for( int idet = 0; idet < NDET; ++idet ) {
      int ndata;
      double data[MAXDATA];
      if( idet == 1 ) {
	// Module number 2 has 4-8 data points (for linear fit)
	ndata = int(5.*drand48())+4;
	double slope = (2.0*drand48()-1.0);
	double inter = (2.0*drand48()-1.0);
	for( int i = 0; i<ndata; ++i ) {
	  assert(2*i+1 < MAXDATA);
	  // y = error + intercept + slope*x;
	  double y1, y2;
	  gauss(y1, y2);
	  double x = i - 3.5 + drand48();
	  data[2*i] = x;
	  data[2*i+1] = y1/20. + inter + slope*x;
	}
	ndata *= 2;
      } else {
	// Generate between 1 and MAXDATA data values per module
	ndata = int(MAXDATA*drand48())+1;
	for( int i = 0; i<ndata; ++i ) {
	  data[i] = 20.0*drand48() - 10.0;
	}
      }
      // Fill module header
      ModuleHeader modhdr;
      // Module number starts counting at 1
      modhdr.module_number = idet+1;
      modhdr.module_ndata = ndata;
      // Calculate module data size
      modhdr.module_length = sizeof(modhdr) + ndata*sizeof(data[0]);
      size_t size_now = modhdr.module_length + evtp-(char*)&evbuffer[0];
      if( size_now > SIZE*sizeof(evbuffer[0]) ) {
	cerr << "Event too large, nev = " << iev
	     << ", size = " << size_now << endl;
	fclose(file);
	exit(2);
      }
      // Write module info to eventbuffer
      memcpy( evtp, &modhdr, sizeof(modhdr) );
      evtp += sizeof(modhdr);
      memcpy( evtp, data, ndata*sizeof(data[0]) );
      evtp += ndata*sizeof(data[0]);
    }
    // Calculate total event length
    evbuffer[0] = evtp-(char*)&evbuffer[0];
    // Write the buffer to file
    size_t c = fwrite( evbuffer, 1, evbuffer[0], file );
    if( c != evbuffer[0] ) {
      cerr << "File write error at event " << iev << endl;
      fclose(file);
      exit(2);
    }
  }

  fclose(file);
  return 0;
}
