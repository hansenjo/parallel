// Interface for reading a data file for parallelization test
// Much simplified version of CodaFile

#include <stdint.h>
#include <cstdio>
#include <string>

class DataFile {
public:
  DataFile( const char* filename = "" );
  ~DataFile();

  bool      IsOpen() { return (filep != 0); }
  int       Open( const char* filename = "" );
  int       Close();

  uint32_t* GetEvBuffer() const { return buffer; }
  size_t    GetBufLen()   const { return buflen; }
  int       ReadEvent();

private:

  std::string filename;
  FILE*       filep;
  uint32_t*   buffer;   // Buffer for current event
  size_t      buflen;   // Number of valid bytes in buffer
};
