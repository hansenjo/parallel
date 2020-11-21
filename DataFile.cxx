// Interface for reading a data file for parallelization test

#include "DataFile.h"
#include <iostream>
#include <memory>

using namespace std;

DataFile::DataFile( string fname ) : filename(move(fname)), filep(nullptr)
{
  // Constructor

  buffer = make_unique<evbuf_t[]>(MAX_EVTSIZE);
  buffer[0] = 0;
}

DataFile::~DataFile()
{
  // Destructor

  Close();
}

int DataFile::Open( const string& fname )
{
  if( !fname.empty() )
    filename = fname;

  filep = fopen( filename.c_str(), "r" );
  if( !filep ) {
    cerr << "Error opening file " << filename << endl;
    return -1;
  }

  return 0;
}

int DataFile::Close()
{
  if( filep ) {
    fclose(filep);
    filep = nullptr;
  }
  return 0;
}

int DataFile::ReadEvent()
{
  if( int status; !IsOpen() && (status = Open()) != 0 )
    return status;

  clearerr(filep);

  const size_t wordsize = sizeof(buffer[0]);
  evbuf_t* bufptr = buffer.get();

  // Read header
  if( fread( bufptr, 1, wordsize, filep ) != wordsize ) {
    if( feof(filep) )
      return -1;
    cerr << "Error reading event header from file " << filename << endl;
    return 1;
  }
  evbuf_t evsize = buffer[0];
  if( evsize > MAX_EVTSIZE*wordsize ) {
    cerr << "Event too large, size = " << evsize << endl;
    return 2;
  }

  // Read data
  if( fread( bufptr+1, 1, evsize-wordsize, filep ) != evsize-wordsize ) {
    // EOF should never occur in the midst of the data
    cerr << "Error reading event data from file " << filename << endl;
    return 2;
  }

  return 0;
}
