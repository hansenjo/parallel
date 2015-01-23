// Generate and write test data file

#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <iostream>

using namespace std;

const char* filename = "test.dat";
const int SIZE = 256, NEVT = 10000;
const long int seed = 87934;

int main( int argc, char** argv )
{
  FILE* file = fopen(filename, "wb");
  if( !file ) {
    cerr << "Cannot open file " << filename << endl;
    exit(1);
  }

  srand48(seed);
  uint32_t evbuffer[SIZE];
  size_t hdrlen = 2;

  for( int iev = 0; iev < NEVT; ++iev ) {
    //Write between 1 and 10  data values per event
    uint32_t evlength = int(10.0*drand48())+1;
    // Event length in bytes, including header
    evbuffer[0] = hdrlen*sizeof(evbuffer[0]) + evlength*sizeof(double);
    evbuffer[1] = evlength;

    // Fill event with data values between -10 and +10
    for( uint32_t i = 0; i<evlength; ++i ) {
      double x = 20.0*drand48() - 10.0;
      size_t stepsize = sizeof(x)/sizeof(evbuffer[0]);
      memcpy( evbuffer+hdrlen+stepsize*i, &x, sizeof(x) );
    }
    // Write the buffer
    size_t c = fwrite( evbuffer, 1, evbuffer[0], file );
    if( c < 1 ) {
      cerr << "File write error at event " << iev << endl;
      fclose(file);
      exit(2);
    }
  }
  
  fclose(file);
  return 0;
}
