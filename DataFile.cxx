// Interface for reading a data file for parallelization test

#include "DataFile.h"
#include <iostream>

using namespace std;

const int BUFSIZE = 256;

DataFile::DataFile( const char* fname )
  : filename(fname), filep(0), buflen(0)
{
  // Constructor

  buffer = new uint32_t[BUFSIZE];
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
  buflen = 0;
  return 0;
}

int DataFile::ReadEvent()
{
  const size_t hdrlen = 2;

  int status = 0;
  if( !IsOpen() && (status = Open()) != 0 )
    return status;

  // Read header
  buflen = 0;
  size_t c = fread( buffer, hdrlen, sizeof(buffer[0]), filep );
  if( c != hdrlen ) {
    cerr << "Error reading event header from file " << filename << endl;
    return 1;
  }
  buflen += hdrlen*sizeof(buffer[0]);
  uint32_t evsize = buffer[0];
  uint32_t evlength = buffer[1];
  if( evsize > BUFSIZE*sizeof(buffer[0]) ) {
    cerr << "Event too large, size = " << evsize << endl;
    return 2;
  }

  // Read data
  c = fread( buffer+hdrlen, evlength, sizeof(double), filep );
  if( c != evlength ) {
    cerr << "Error reading event data from file " << filename << endl;
    return 2;
  }
  buflen += evlength*sizeof(double);

  return status;
}
