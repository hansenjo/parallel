// Interface for reading a data file for parallelization test
// Much simplified version of CodaFile

#ifndef PPODD_DATAFILE
#define PPODD_DATAFILE

#include <cstdint>
#include <cstdio>
#include <string>

using evbuf_t = uint32_t;
using evbuf_ptr_t = std::unique_ptr<evbuf_t[]>;
static const int MAX_EVTSIZE = 1024;

class DataFile {
public:
  explicit DataFile( const char* filename = "" );
  ~DataFile();

  bool      IsOpen() { return (filep != nullptr); }
  int       Open( const char* filename = "" );
  int       ReadEvent();
  int       Close();

  [[nodiscard]] evbuf_t*  GetEvBufPtr() const { return buffer.get(); }
  [[nodiscard]] evbuf_ptr_t& GetEvBuffer()    { return buffer; }
  [[nodiscard]] evbuf_t   GetEvSize()   const { return buffer[0]; }
  [[nodiscard]] size_t    GetEvWords()  const { return GetEvSize()/sizeof(evbuf_t); }

private:

  std::string filename;
  FILE*       filep;
  evbuf_ptr_t buffer;   // Buffer for current event
};

#endif
