// Simple decoder for event buffer read from test data file

#ifndef PPODD_DECODER
#define PPODD_DECODER

#include "rawdata.h"
#include "DataFile.h" // for evbuf_t
#include <vector>
#include <cassert>

class Decoder {
public:
  Decoder();

  int Load( evbuf_t* evbuffer );

  int     GetEvSize()       const { return event.header.event_length; }
  int     GetNdata( int m ) const;
  double  GetData( int m, int i ) const;
  double* GetDataBuf( int m ) const;

private:
  Event event;

  void Clear();
};


inline
int Decoder::GetNdata( int m ) const
{
  if( event.module[m] == 0 )
    return 0;

  return event.module[m]->header.module_ndata;
}

inline
double Decoder::GetData( int m, int i ) const
{
  assert( event.module[m] );
  return event.module[m]->data[i];
}

inline
double* Decoder::GetDataBuf( int m ) const
{
  assert( event.module[m] );
  return event.module[m]->data;
}

#endif
