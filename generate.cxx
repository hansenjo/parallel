// Generate and write test data file
//
// For the file format generated, see rawdata.h

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <iostream>
#include <unistd.h>
#include <cassert>
#include <tuple>
#include <stdexcept>
//#include <vector>
#include <memory>
#include <sstream>

#include "rawdata.h"

using namespace std;

// Configuration
static constexpr size_t SIZE = 1024;
static constexpr long int seed = 87934;
struct Config {
  const char* prgname{""};
  const char* filename{""};
  int debug{0};
  unsigned NEVT{10000};
  unsigned NDET{1};
};
static Config conf;

// Gaussian-distributed random numbers
static inline
tuple<double,double> gauss()
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

  return make_tuple(x1*w, x2*w);
}

// Usage message
static void usage()
{
  cerr << "Usage: " << conf.prgname << " [options] output_file" << endl
       << "where options are:" << endl
       << " [ -c num ]\tnumber of detectors to simulate (default 1)" << endl
       << " [ -n nev_max ]\t\tset number of events (default 10000)" << endl
       << " [ -d debug_level ]\tset debug level (default 0)" << endl
       << " [ -h ]\t\t\tprint this help message" << endl;
  exit(255);
}

// Eventbuffer class
class EventBuffer {
public:
  using EvBuf_t = uint32_t;

  explicit EventBuffer(size_t size) :
          m_nwords(size),
          m_wordsz(sizeof(EvBuf_t)),
          m_buf(new EvBuf_t[size]),
          m_bufstart((char*)m_buf.get()),
          m_evtp(m_bufstart)
  {}

  void fill_header(uint32_t ndet) {
    EventHeader evthdr(0, ndet);
    m_evtp = m_bufstart;
    memcpy(m_evtp, &evthdr, sizeof(evthdr) );
    m_evtp += sizeof(evthdr);
  }
  void append_module(uint32_t idet, uint32_t ndata, EvDat_t* data) {
    ModuleHeader modhdr( sizeof(modhdr) + ndata*sizeof(EvDat_t),
                         idet+1, // in the data file, module numbers start counting at 1
                         ndata );
    size_t size_now = modhdr.module_length + m_evtp - m_bufstart;
    if( size_now > m_nwords * m_wordsz ) {
      ostringstream ostr;
      ostr << "Event too large, size = " << size_now << endl;
      throw runtime_error(ostr.str());
    }
    // Append module header and data array to eventbuffer
    memcpy(m_evtp, &modhdr, sizeof(modhdr) );
    m_evtp += sizeof(modhdr);
    memcpy(m_evtp, data, ndata * sizeof(EvDat_t) );
    m_evtp += ndata * sizeof(EvDat_t);
  }
  void write( FILE* file ) {
    // Calculate total event length
    auto *buf = (EvBuf_t*)m_bufstart;
    buf[0] = m_evtp-m_bufstart;
    // Write the buffer to file
    size_t c = fwrite( buf, 1, buf[0], file );
    if( c != buf[0] ) {
      throw runtime_error("File write error");
    }
  }
private:
  const size_t m_nwords;
  const size_t m_wordsz;
  unique_ptr<uint32_t> m_buf;
  char* m_bufstart;
  char* m_evtp;
};

// Command line parser
void get_args( int argc, char** argv )
{
  conf.prgname = argv[0];
  if( strlen(conf.prgname) >= 2 && strncmp(conf.prgname,"./",2) == 0 )
    conf.prgname += 2;

  int opt;
  while( (opt = getopt(argc, argv, "c:d:n:h")) != -1 ) {
    switch (opt) {
      case 'c':
        conf.NDET = stoi(optarg);
        if( conf.NDET > MAXMODULES ) {
          cerr << "Too many detectors, max " << MAXMODULES << endl;
          exit(255);
        }
        break;
      case 'd':
        conf.debug = stoi(optarg);
        break;
      case 'n':
        conf.NEVT = stoi(optarg);
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
  conf.filename = argv[optind];
}

int main( int argc, char** argv )
{
  get_args(argc, argv);

  // Open output
  FILE* file = fopen(conf.filename, "wb");
  if( !file ) {
    cerr << "Cannot open file " << conf.filename << endl;
    exit(1);
  }

  srand48(seed);
  EventBuffer evbuffer(SIZE);

  try {
    // Generate event data
    for( unsigned iev = 0; iev < conf.NEVT; ++iev ) {
      evbuffer.fill_header(conf.NDET);
      for( unsigned idet = 0; idet < conf.NDET; ++idet ) {
        unsigned ndata;
        EvDat_t data[MAXDATA];
        switch( idet ) {
          case 1:
            // Module type 2 wants 4-8 data points for linear fit
            ndata = unsigned(5. * drand48()) + 4;
            {
              double slope = (2.0 * drand48() - 1.0);
              double inter = (2.0 * drand48() - 1.0);
              for( unsigned i = 0; i < ndata; ++i ) {
                assert(2 * i + 1 < MAXDATA);
                // y = error + intercept + slope*x;
                auto [y1,y2] = gauss();
                double x = i - 3.5 + drand48();
                data[2 * i] = x;
                data[2 * i + 1] = y1 / 20. + inter + slope * x;
              }
            }
            ndata *= 2;
            break;
          case 2:
            // Module type 3 wants a single data word indicating the desired precision
            ndata = 1;
            data[0] = 0.0;
            while( data[0] < 300. ) {
              auto [y1,y2] = gauss();
              data[0] = 2000. + 200. * y1;
            }
            break;
          default:
            // Generate between 1 and MAXDATA random data values per module
            ndata = unsigned(MAXDATA * drand48()) + 1;
            for( unsigned i = 0; i < ndata; ++i ) {
              data[i] = 20.0 * drand48() - 10.0;
            }
            break;
        }

        evbuffer.append_module(idet, ndata, data);
      }
      evbuffer.write(file);
    }
  }
  catch ( const exception& e ) {
    cerr << "Error while generating events: " << e.what() << endl;
    fclose(file);
    return 1;
  }

  if( fclose(file) != 0) {
    cerr << "Error writing output file " << conf.filename << endl;
    return 1;
  }
  cout << "Successfully generated " << conf.NEVT << " events for "
       << conf.NDET << " detectors" << endl;

  return 0;
}
