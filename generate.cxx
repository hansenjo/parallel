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

using namespace std;

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
      ModuleHeader modhdr;
      double data[MAXDATA];
      // Module number starts counting at 1
      modhdr.module_number = idet+1;
      // Generate between 1 and MAXDATA data values per module
      int ndata = int(MAXDATA*drand48())+1;
      modhdr.module_ndata = ndata;
      // Fill event with data values between -10 and +10
      for( int i = 0; i<ndata; ++i )
	data[i] = 20.0*drand48() - 10.0;
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
