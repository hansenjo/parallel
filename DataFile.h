// Interface for reading a data file for parallelization test
// Much simplified version of CodaFile

#ifndef PPODD_DATAFILE
#define PPODD_DATAFILE

#include <cstdint>
#include <cstdio>
#include <string>

typedef uint32_t evbuf_t;

class DataFile {
public:
  explicit DataFile( const char* filename = "" );
  ~DataFile();

  bool      IsOpen() { return (filep != 0); }
  int       Open( const char* filename = "" );
  int       ReadEvent();
  int       Close();

  [[nodiscard]] evbuf_t*  GetEvBuffer() const { return buffer; }
  [[nodiscard]] evbuf_t*& GetEvBuffer()       { return buffer; }
  [[nodiscard]] evbuf_t   GetEvSize()   const { return buffer[0]; }
  [[nodiscard]] size_t    GetEvWords()  const { return GetEvSize()/sizeof(evbuf_t); }

private:

  std::string filename;
  FILE*       filep;
  evbuf_t*    buffer;   // Buffer for current event
};

#endif
