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
  int Preload( evbuf_t* evbuffer );

  [[nodiscard]] uint32_t GetEvSize()  const { return event.header.event_length; }
  [[nodiscard]] uint32_t GetNdata( int m ) const;
  [[nodiscard]] double   GetData( uint32_t m, uint32_t i ) const;
  [[nodiscard]] double*  GetDataBuf( uint32_t m ) const;
  [[nodiscard]] bool     IsSyncEvent() const;

private:
  Event event;

  void Clear();
};


inline
uint32_t Decoder::GetNdata( int m ) const
{
  if( event.module[m] == nullptr )
    return 0;

  return event.module[m]->header.module_ndata;
}

inline
double Decoder::GetData( uint32_t m, uint32_t i ) const
{
  assert( event.module[m] );
  return event.module[m]->data[i];
}

inline
double* Decoder::GetDataBuf( uint32_t m ) const
{
  assert( event.module[m] );
  return event.module[m]->data;
}

#endif
