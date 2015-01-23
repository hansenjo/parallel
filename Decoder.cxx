// Interface for reading a data file for parallelization test

#include "Decoder.h"
#include <iostream>
#include <cstring>

using namespace std;

Decoder::Decoder() : evsize(0)
{
  data.reserve(10);
}

void Decoder::Clear()
{
  evsize = 0;
  data.clear();
}

int Decoder::Load( uint32_t* evbuffer )
{
  Clear();

  if( !evbuffer )
    return 1;

  evsize = evbuffer[0];
  if( evsize < 8 )
    return 2;
  uint32_t ndata = evbuffer[1];
  if( ndata > 0 ) {
    double x;
    for( uint32_t i = 0; i < ndata; ++i ) {
      const size_t stepsize = sizeof(x)/sizeof(evbuffer[0]);
      memcpy( &x, evbuffer+2+stepsize*i, sizeof(double) );
      data.push_back(x);
    }
  }
  return 0;
}


