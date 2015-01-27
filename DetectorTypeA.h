// Simple detector example class
//
// This detector type computes basic statistics of the raw data

#ifndef PPODD_DETECTORA
#define PPODD_DETECTORA

#include "Detector.h"

class Decoder;

class DetectorTypeA : public Detector {
public:
  DetectorTypeA( const char* name, int imod );
  virtual ~DetectorTypeA();

  virtual void Clear();
  virtual int  Decode( Decoder& evdata );
  virtual int  Analyze();
  virtual void Print() const;

protected:
  // Statistics results
  double nval;
  double sum;
  double min;
  double max;
  double mean;
  double geom;

  virtual int  DefineVariables( bool remove = false );
};

#endif
