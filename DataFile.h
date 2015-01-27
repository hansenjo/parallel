// Interface for reading a data file for parallelization test
// Much simplified version of CodaFile

#ifndef PPODD_DATAFILE
#define PPODD_DATAFILE

#include <stdint.h>
#include <cstdio>
#include <string>

typedef uint32_t evbuf_t;

class DataFile {
public:
  DataFile( const char* filename = "" );
  ~DataFile();

  bool      IsOpen() { return (filep != 0); }
  int       Open( const char* filename = "" );
  int       Close();

  evbuf_t*  GetEvBuffer() const { return buffer; }
  evbuf_t   GetEvSize()   const { return buffer[0]; }
  size_t    GetEvWords()  const { return GetEvSize()/sizeof(evbuf_t); }
  int       ReadEvent();

private:

  std::string filename;
  FILE*       filep;
  evbuf_t*    buffer;   // Buffer for current event
};

#endif
