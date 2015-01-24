// Simple decoder for event buffer read from test data file

#ifndef PPODD_DECODER

#include <stdint.h>
#include <vector>

class Decoder {
public:
  Decoder();

  int Load( uint32_t* evbuffer );

  int    GetEvSize()      const  { return evsize; }
  int    GetNdata()       const  { return data.size(); }
  double GetData( int i ) const  { return data[i]; }

private:
  int evsize;
  std::vector<double> data;

  void Clear();
};

#endif
