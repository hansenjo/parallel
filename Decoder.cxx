// Interface for reading a data file for parallelization test

#include "Decoder.h"
#include <cstring>  // for memset, memcpy

using namespace std;

Decoder::Decoder() = default;

void Decoder::Clear()
{
  memset( event.module, 0, sizeof(event.module) );
}

int Decoder::Load( evbuf_t* evbuffer )
{
  int status = Preload( evbuffer );
  if( status )
    return status;

  Clear();

  char* evtp = ((char*)evbuffer)+sizeof(event.header);
  int ndet = static_cast<int>(event.header.event_info & 0xFFFFU);
  for( int i = 0; i < ndet; ++i ) {
    auto* m = (ModuleData*)evtp;
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

int Decoder::Preload( evbuf_t* evbuffer )
{
  if( !evbuffer )
    return 1;
  if( evbuffer[0] < 8 )
    return 2;

  memcpy( &event.header, evbuffer, sizeof(event.header) );

  return 0;
}

bool Decoder::IsSyncEvent() const
{
  return ((event.header.event_info & 0x10000U) != 0);
}
