// Simple detector class

#include "Detector.h"
#include "Decoder.h"
#include <iostream>

using namespace std;

Detector::Detector( const char* _name ) : name(_name)
{
}

void Detector::Clear()
{
}

int Detector::Init()
{
  return 0;
}

int Detector::Decode( Decoder& evdata )
{

  int ndata = evdata.GetNdata();
  cout << "DET: " << name << ", ndata = " << ndata;
  if( ndata > 0 ) {
    cout << ", data = ";
    for( int i = 0; i < ndata; ++i ) {
      cout << evdata.GetData(i);
      if( i+1 != ndata )
	cout << ", ";
    }
  }
  cout << endl;

  return 0;
}

int Detector::Analyze( Decoder& /* evdata */ )
{

  return 0;
}

void Detector::Print() const
{

}
