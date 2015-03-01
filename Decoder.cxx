// Interface for reading a data file for parallelization test

#include "Decoder.h"
#include <iostream>
#include <cstring>  // for memset, memcpy

using namespace std;

Decoder::Decoder()
{
}

void Decoder::Clear()
{
  memset( event.module, 0, sizeof(event.module) );
}

int Decoder::Load( evbuf_t* evbuffer )
{
  Clear();

  if( !evbuffer )
    return 1;
  if( evbuffer[0] < 8 )
    return 2;

  memcpy( &event.header, evbuffer, sizeof(event.header) );
  char* evtp = ((char*)evbuffer)+sizeof(event.header);
  int ndet = event.header.event_info;
  for( int i = 0; i < ndet; ++i ) {
    ModuleData* m = (ModuleData*)evtp;
    if( !m )
      return 3;
    int imod = m->header.module_number;
    if( imod < 1 )
      return 4;
    event.module[imod-1] = m;
    evtp += m->header.module_length;
  }
  return 0;
}
