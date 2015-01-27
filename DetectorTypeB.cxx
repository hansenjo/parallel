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
  // This detector type performs a linear fit to the data

  typedef vector<double> vec_t;
  if( data.size() >= 3 ) {
    double S11 = 0, S12 = 0, S22 = 0, G1 = 0, G2 = 0;
    for( vec_t::size_type i = 0; i < data.size(); ++i ) {
      double x = 1.2*double(i)-4.0;
      double y = data[i];
      S11 += 1.0;
      S12 += x;
      S22 += x*x;
      G1  += y;
      G2  += x*y;
    }
    double D = 1.0 / (S11*S22 - S12*S12);
    inter = (G1*S22 - G2*S12)*D;
    slope = (G2*S11 - G1*S12)*D;
    cov11 = S11*D;
    cov22 = S22*D;
    cov12 = -S12*D;
    chi2 = 0;
    for( vec_t::size_type i = 0; i < data.size(); ++i ) {
      double x = 1.2*double(i)-4.0;
      double d = inter + slope*x;
      chi2 += d*d;
    }
    ndof = double(data.size())-2.0;
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
    { "slope", "Slope",                &slope },
    { "inter", "Intercept",            &inter },
    { "cov11", "Error^2 in slope",     &cov11 },
    { "cov22", "Error^2 in intercept", &cov22 },
    { "cov12", "Correlation coeff",    &cov12 },
    { "ndof",  "Degrees of freedom",   &ndof },
    { "chi2",  "Chi2",                 &chi2 },
    { 0 }
  };
  DefineVarsFromList( defs, GetName().c_str(), gVars, do_remove );
  return 0;
}
