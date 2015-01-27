// Another simple detector class

#include "Podd.h"
#include "DetectorTypeB.h"
#include <iostream>
#include <cmath>

using namespace std;

DetectorTypeB::DetectorTypeB( const char* name, int imod ) : Detector(name,imod)
{
  type = "B";
}

DetectorTypeB::~DetectorTypeB()
{
  DefineVariables( kRemove );
}

void DetectorTypeB::Clear()
{
  Detector::Clear();
  ndof = 0;
  slope = inter = cov11 = cov22 = cov12 = chi2 = 1e38;
}

int DetectorTypeB::Analyze()
{
  // This detector type computes some basic statistics of the raw data

  typedef vector<double> vec_t;
  if( !data.empty() ) {
    double n = double(data.size());
    for( vec_t::size_type i = 0; i < data.size(); ++i ) {
      double x = data[i];
      // TODO
    }
  }

  return 0;
}

void DetectorTypeB::Print() const
{
  Detector::Print();
}

int DetectorTypeB::DefineVariables( bool do_remove )
{
  VarDef_t defs[] = {
    { "slope", "Slope",              &slope },
    { "inter", "Intercept",          &inter },
    { "cov11", "Error in slope",     &cov11 },
    { "cov22", "Error in intercept", &cov22 },
    { "cov12", "Correlation coeff",  &cov12 },
    { "ndof",  "Degrees of freedom", &ndof },
    { "chi2",  "Chi2",               &chi2 },
    { 0 }
  };
  DefineVarsFromList( defs, GetName().c_str(), gVars, do_remove );
  return 0;
}
