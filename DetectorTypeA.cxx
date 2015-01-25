// Simple detector class

#include "Podd.h"
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
  nval = 0;
  sum = mean = geom = 0;
  min = 1e38;
  max = -min;
}

int DetectorTypeA::Decode( Decoder& evdata )
{
  int ndata = evdata.GetNdata();
  if( debug > 1 )
    Print();
  if( debug > 2 )
    cout << " Decode: ndata = " << ndata;
  if( ndata > 0 ) {
    if( debug > 3 )
      cout << ", data = ";
    for( int i = 0; i < ndata; ++i ) {
      data.push_back(evdata.GetData(i));
      if( debug > 3 ) {
	cout << evdata.GetData(i);
	if( i+1 != ndata )
	  cout << ", ";
      }
    }
  }
  if( debug > 2 )
    cout << endl;

  nval = ndata;

  return 0;
}

int DetectorTypeA::Analyze()
{
  // This detector type computes some basic statistics of the raw data

  typedef vector<double> vec_t;
  if( !data.empty() ) {
    double n = double(data.size());
    for( vec_t::size_type i = 0; i < data.size(); ++i ) {
      double x = data[i];
      sum += x;
      if( x < min ) min = x;
      if( x > max ) max = x;
      geom += log(fabs(x));
    }
    mean = sum/n;
    geom = exp(geom/n);
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
    { "nval", "Number of data values processed", &nval },
    { "sum",  "Sum of data",            &sum },
    { "min",  "Minimum of data",        &min },
    { "max",  "Maximum of data",        &max },
    { "mean", "Mean of data",           &mean },
    { "geom", "Geometric mean of data", &geom },
    { 0 }
  };
  DefineVarsFromList( defs, GetName().c_str(), gVars, do_remove );
  return 0;
}
