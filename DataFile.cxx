// Interface for reading a data file for parallelization test

#include "DataFile.h"
#include <iostream>

using namespace std;

const int BUFSIZE = 1024;

DataFile::DataFile( const char* fname ) : filename(fname), filep(0)
{
  // Constructor

  buffer = new evbuf_t[BUFSIZE];
  buffer[0] = 0;
}

DataFile::~DataFile()
{
  // Destructor

  Close();
  delete [] buffer;
}

int DataFile::Open( const char* fname )
{
  if( fname && *fname )
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
    filep = 0;
  }
  return 0;
}

int DataFile::ReadEvent()
{
  const size_t wordsize = sizeof(buffer[0]);

  int status = 0;
  if( !IsOpen() && (status = Open()) != 0 )
    return status;

  clearerr(filep);

  // Read header
  size_t c = fread( buffer, 1, wordsize, filep );
  if( c != wordsize ) {
    if( feof(filep) )
      return -1;
    cerr << "Error reading event header from file " << filename << endl;
    return 1;
  }
  evbuf_t evsize = buffer[0];
  if( evsize > BUFSIZE*wordsize ) {
    cerr << "Event too large, size = " << evsize << endl;
    return 2;
  }

  // Read data
  c = fread( buffer+1, 1, evsize-wordsize, filep );
  if( c != evsize-wordsize ) {
    // EOF should never occur in the midst of the data
    cerr << "Error reading event data from file " << filename << endl;
    return 2;
  }

  return status;
}
