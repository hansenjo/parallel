// Simple detector class

#include "DetectorTypeA.h"
#include "Decoder.h"
#include <iostream>

using namespace std;

DetectorTypeA::DetectorTypeA( const char* _name ) : Detector(_name)
{
  type = "A";
}

void DetectorTypeA::Clear()
{
}

int DetectorTypeA::Init()
{
  return 0;
}

int DetectorTypeA::Decode( Decoder& evdata )
{

  int ndata = evdata.GetNdata();
  Print();
  cout << " Decode: ndata = " << ndata;
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

int DetectorTypeA::Analyze( Decoder& /* evdata */ )
{

  return 0;
}

void DetectorTypeA::Print() const
{
  Detector::Print();
}

int DetectorTypeA::DefineVariables( bool )
{
  return 0;
}
