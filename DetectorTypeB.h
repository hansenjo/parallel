// Another simple detector example class
//
// This detector type performs a linear fit to the data points

#ifndef PPODD_DETECTORB
#define PPODD_DETECTORB

#include "Detector.h"

class Decoder;

class DetectorTypeB : public Detector {
public:
  DetectorTypeB( const char* name, int imod );
  virtual ~DetectorTypeB();

  virtual void Clear();
  virtual int  Analyze();
  virtual void Print() const;

protected:
  // Fit results
  double slope;
  double inter;
  double cov11;
  double cov22;
  double cov12;
  double ndof;
  double chi2;

  virtual int  DefineVariables( bool remove = false );
};

#endif
