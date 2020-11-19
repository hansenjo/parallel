// Simple detector class

#include "Podd.h"
#include "DetectorTypeA.h"
#include "Decoder.h"
#include <cmath>

using namespace std;

DetectorTypeA::DetectorTypeA( const char* name, int imod ) : Detector(name,imod)
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
  nval = 0;
  sum = mean = geom = 0;
  min = 1e38;
  max = -min;
}

Detector* DetectorTypeA::Clone() const
{
  return new DetectorTypeA(*this);
}


int DetectorTypeA::Decode( Decoder& evdata )
{
  int status = Detector::Decode( evdata );
  if( status )
    return status;

  nval = evdata.GetNdata(imod);

  return 0;
}

int DetectorTypeA::Analyze()
{
  // This detector type computes some basic statistics of the raw data

  if( !data.empty() ) {
    auto n = double(data.size());
    for( double x : data ) {
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
  DefineVarsFromList( defs, GetName().c_str(), fVars, do_remove );
  return 0;
}
