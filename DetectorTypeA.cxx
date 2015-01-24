// Simple detector class

#include "DetectorTypeA.h"
#include "Decoder.h"
#include <iostream>
#include <cmath>

using namespace std;

DetectorTypeA::DetectorTypeA( const char* _name ) : Detector(_name)
{
  type = "A";
}

DetectorTypeA::~DetectorTypeA()
{
  DefineVariables( kRemove );
}

void DetectorTypeA::Clear()
{
  Detector::Clear();
  data.clear();
  sum = mean = geom = 0;
  min = 1e38;
  max = -min;
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
      data.push_back(evdata.GetData(i));
    }
  }
  cout << endl;

  return 0;
}

int DetectorTypeA::Analyze()
{
  // This detector type compute some basic statistics of the raw data

  typedef vector<double> vec_t;
  if( !data.empty() ) {
    double n = double(data.size());
    geom = 1.0;
    for( vec_t::size_type i = 0; i < data.size(); ++i ) {
      double x = data[i];
      sum += x;
      if( x < min ) min = x;
      if( x > max ) max = x;
      geom *= x;
    }
    mean = sum/n;
    geom = pow(fabs(geom), 1./n);
  }

  return 0;
}

void DetectorTypeA::Print() const
{
  Detector::Print();
}

int DetectorTypeA::DefineVariables( bool do_remove )
{
  VarDef_t defs[] = {
    { "sum",  "Sum of data",            &sum },
    { "min",  "Minimum of data",        &min },
    { "max",  "Maximum of data",        &max },
    { "mean", "Mean of data",           &mean },
    { "geom", "Geometric mean of data", &geom },
    { 0 }
  };
  DefineVarsFromList( defs, GetName().c_str(), do_remove );
  return 0;
}
