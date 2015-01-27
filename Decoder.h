// Simple decoder for event buffer read from test data file

#ifndef PPODD_DECODER
#define PPODD_DECODER

#include "rawdata.h"
#include "DataFile.h" // for evbuf_t
#include <vector>

class Decoder {
public:
  Decoder();

  int Load( evbuf_t* evbuffer );

  int    GetEvSize()       const { return event.header.event_length; }
  int    GetNdata( int m ) const;
  double GetData( int m, int i ) const;

private:
  Event event;

  void Clear();
};

#endif
